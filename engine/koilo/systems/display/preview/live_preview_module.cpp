// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file live_preview_module.cpp
 * @brief Implementation of the MJPEG live preview kernel module.
 *
 * @date 04/06/2026
 * @author Coela Can't
 */

#ifdef KL_HAVE_LIVE_PREVIEW

#include <koilo/systems/display/preview/live_preview_module.hpp>
#include <koilo/kernel/kernel.hpp>
#include <koilo/kernel/cvar/cvar_system.hpp>
#include <koilo/kernel/console/console_module.hpp>
#include <koilo/kernel/logging/log.hpp>
#include <koilo/systems/render/pipeline/render_pipeline.hpp>
#include <koilo/systems/render/rhi/rhi_device.hpp>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include <algorithm>
#include <cstring>
#include <cstdio>
#include <sstream>

namespace koilo {

// -- CVars --------------------------------------------------------------------

static AutoCVar_Bool cvar_previewEnabled("preview.enabled",
    "Enable live preview HTTP server", true);

static AutoCVar_Int cvar_previewPort("preview.port",
    "Live preview HTTP port", 8080);

static AutoCVar_Int cvar_previewFps("preview.fps",
    "Max preview stream frame rate", 15);

static AutoCVar_Int cvar_previewQuality("preview.quality",
    "Preview quality (reserved for future JPEG support)", 70);

// -- Embedded HTML viewer -----------------------------------------------------

static const char* kHTMLPage = R"html(<!DOCTYPE html>
<html>
<head>
<title>Koilo Live Preview</title>
<style>
body{background:#111;margin:0;display:flex;flex-direction:column;
align-items:center;justify-content:center;height:100vh;
font-family:monospace;color:#aaa}
img{image-rendering:pixelated;max-width:95vw;max-height:90vh;
border:1px solid #333}
#info{margin:8px;font-size:12px}
</style>
</head>
<body>
<div id="info">Koilo Engine - Live Preview</div>
<img id="preview" src="/stream" alt="Connecting...">
<script>
const img=document.getElementById('preview');
const info=document.getElementById('info');
let frames=0,last=performance.now();
img.onload=()=>{frames++;
const now=performance.now();
if(now-last>1000){info.textContent=''+Math.round(frames*1000/(now-last))+' fps '
+img.naturalWidth+'x'+img.naturalHeight;
frames=0;last=now;}};
img.onerror=()=>{info.textContent='Connection lost - reconnecting...';
setTimeout(()=>{img.src='/stream?t='+Date.now();},2000);};
</script>
</body>
</html>)html";

// -- Static instance ----------------------------------------------------------

std::unique_ptr<LivePreviewModule> LivePreviewModule::instance_;

// -- Helpers ------------------------------------------------------------------

static void SetNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

static void SetNoDelay(int fd) {
    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
}

static void WriteLE16(uint8_t* p, uint16_t v) {
    p[0] = static_cast<uint8_t>(v);
    p[1] = static_cast<uint8_t>(v >> 8);
}

static void WriteLE32(uint8_t* p, uint32_t v) {
    p[0] = static_cast<uint8_t>(v);
    p[1] = static_cast<uint8_t>(v >> 8);
    p[2] = static_cast<uint8_t>(v >> 16);
    p[3] = static_cast<uint8_t>(v >> 24);
}

// -- BMP encoder --------------------------------------------------------------

void LivePreviewModule::EncodeBMP(const uint8_t* rgba, uint32_t width,
                                   uint32_t height,
                                   std::vector<uint8_t>& out) {
    const uint32_t rowBytes = width * 3;
    const uint32_t paddedRow = (rowBytes + 3) & ~3u;
    const uint32_t imgSize = paddedRow * height;
    const uint32_t fileSize = 54 + imgSize;

    out.assign(fileSize, 0);
    uint8_t* p = out.data();

    // BITMAPFILEHEADER (14 bytes)
    p[0] = 'B'; p[1] = 'M';
    WriteLE32(p + 2, fileSize);
    // p+6..p+9 = reserved (0)
    WriteLE32(p + 10, 54);     // pixel data offset
    p += 14;

    // BITMAPINFOHEADER (40 bytes)
    WriteLE32(p, 40);          // header size
    WriteLE32(p + 4, width);
    // Negative height = top-down scanlines (no vertical flip needed)
    int32_t negH = -static_cast<int32_t>(height);
    std::memcpy(p + 8, &negH, 4);
    WriteLE16(p + 12, 1);      // planes
    WriteLE16(p + 14, 24);     // bits per pixel
    // p+16..p+19 = compression (0 = BI_RGB)
    WriteLE32(p + 20, imgSize);
    // p+24..p+39 = resolution/color fields (0)
    p += 40;

    // Pixel data: RGBA -> BGR with row padding
    for (uint32_t y = 0; y < height; ++y) {
        uint8_t* row = p + y * paddedRow;
        const uint8_t* src = rgba + y * width * 4;
        for (uint32_t x = 0; x < width; ++x) {
            row[x * 3 + 0] = src[x * 4 + 2]; // B
            row[x * 3 + 1] = src[x * 4 + 1]; // G
            row[x * 3 + 2] = src[x * 4 + 0]; // R
        }
    }
}

// -- HTTP parsing -------------------------------------------------------------

bool LivePreviewModule::ParseHTTPRequest(const char* data, size_t len,
                                          std::string& outPath) {
    // Minimal GET parser: "GET /path HTTP/1.x\r\n..."
    if (len < 14) return false;
    if (std::strncmp(data, "GET ", 4) != 0) return false;

    const char* pathStart = data + 4;
    const char* pathEnd = static_cast<const char*>(
        std::memchr(pathStart, ' ', len - 4));
    if (!pathEnd) return false;

    // Strip query string
    const char* q = static_cast<const char*>(
        std::memchr(pathStart, '?', static_cast<size_t>(pathEnd - pathStart)));
    if (q) pathEnd = q;

    outPath.assign(pathStart, pathEnd);
    return true;
}

// -- Lifecycle ----------------------------------------------------------------

LivePreviewModule::LivePreviewModule()
    : kernel_(nullptr)
    , listenFd_(-1)
    , running_(false)
    , frameReady_(false)
    , clientCount_(0)
    , framesStreamed_(0) {}

LivePreviewModule::~LivePreviewModule() = default;

const ModuleDesc& LivePreviewModule::GetModuleDesc() {
    static ModuleDesc desc{};
    desc.name         = "koilo.live_preview";
    desc.version      = KL_VERSION(0, 1, 0);
    desc.requiredCaps = Cap::None;
    desc.Init         = &LivePreviewModule::Init;
    desc.Tick         = &LivePreviewModule::Tick;
    desc.OnMessage    = &LivePreviewModule::OnMessage;
    desc.Shutdown     = &LivePreviewModule::Shutdown;
    return desc;
}

bool LivePreviewModule::Init(KoiloKernel& kernel) {
    instance_ = std::make_unique<LivePreviewModule>();
    instance_->kernel_ = &kernel;

    if (!cvar_previewEnabled.Get()) {
        KL_LOG("live_preview", "Live preview disabled via preview.enabled");
        return true;
    }

    int port = cvar_previewPort.Get();

    // Create TCP listener
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        KL_WARN("live_preview", "Failed to create socket: %s",
                 strerror(errno));
        return true; // non-fatal
    }

    int reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(fd, reinterpret_cast<struct sockaddr*>(&addr),
             sizeof(addr)) < 0) {
        KL_WARN("live_preview", "Failed to bind port %d: %s",
                 port, strerror(errno));
        close(fd);
        return true;
    }

    if (listen(fd, 8) < 0) {
        KL_WARN("live_preview", "listen() failed: %s", strerror(errno));
        close(fd);
        return true;
    }

    SetNonBlocking(fd);
    instance_->listenFd_ = fd;
    instance_->running_ = true;
    instance_->serverThread_ = std::thread(
        &LivePreviewModule::ServerThreadFunc, instance_.get());

    instance_->RegisterCommands();
    kernel.Services().Register("live_preview", instance_.get());

    KL_LOG("live_preview",
           "Live preview server started on http://0.0.0.0:%d/", port);
    return true;
}

void LivePreviewModule::Tick(float /*dt*/) {
    if (!instance_ || !instance_->running_) return;
    if (!cvar_previewEnabled.Get()) return;

    // Skip encoding if no one is watching
    if (instance_->clientCount_.load(std::memory_order_relaxed) == 0) return;

    // FPS throttle
    int maxFps = cvar_previewFps.Get();
    if (maxFps < 1) maxFps = 1;
    auto minInterval = std::chrono::microseconds(1000000 / maxFps);
    auto now = Clock::now();
    if (now - instance_->lastFrameTime_ < minInterval) return;

    // Get render pipeline
    auto* pipeline = static_cast<rhi::RenderPipeline*>(
        instance_->kernel_->Services().Get<void>("render_backend"));
    if (!pipeline) return;

    auto* device = pipeline->GetDevice();
    if (!device) return;

    const uint8_t* swPixels = device->GetSwapchainPixels();
    if (!swPixels) return;

    uint32_t fbWidth = 0, fbHeight = 0;
    device->GetSwapchainSize(fbWidth, fbHeight);
    if (fbWidth == 0 || fbHeight == 0) return;

    // Encode BMP into a local buffer, then swap into shared storage
    std::vector<uint8_t> localBmp;
    EncodeBMP(swPixels, fbWidth, fbHeight, localBmp);

    {
        std::lock_guard<std::mutex> lk(instance_->frameMutex_);
        instance_->bmpBuffer_.swap(localBmp);
        instance_->frameReady_ = true;
    }

    instance_->lastFrameTime_ = now;
    ++instance_->framesStreamed_;
}

void LivePreviewModule::OnMessage(const Message& /*msg*/) {}

void LivePreviewModule::Shutdown() {
    if (!instance_) return;

    instance_->running_ = false;

    // Wake up the server thread by closing the listen socket
    if (instance_->listenFd_ >= 0) {
        close(instance_->listenFd_);
        instance_->listenFd_ = -1;
    }

    if (instance_->serverThread_.joinable()) {
        instance_->serverThread_.join();
    }

    // Close all stream clients
    {
        std::lock_guard<std::mutex> lk(instance_->clientsMutex_);
        for (int fd : instance_->streamClients_) {
            close(fd);
        }
        instance_->streamClients_.clear();
    }

    if (instance_->kernel_) {
        auto& svc = instance_->kernel_->Services();
        if (svc.Has("live_preview")) {
            svc.Unregister("live_preview");
        }
    }

    instance_.reset();
}

// -- Reliable send helper -----------------------------------------------------

bool LivePreviewModule::SendAll(int fd, const void* data, size_t len) {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    size_t remaining = len;
    while (remaining > 0) {
        ssize_t n = send(fd, p, remaining, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0) return false;
        p += n;
        remaining -= static_cast<size_t>(n);
    }
    return true;
}

// -- Server thread ------------------------------------------------------------

void LivePreviewModule::ServerThreadFunc() {
    std::vector<uint8_t> sendBuffer;

    while (running_) {
        // Poll for new connections (short timeout so we also check frames)
        struct pollfd pfd{};
        pfd.fd = listenFd_;
        pfd.events = POLLIN;

        int ret = poll(&pfd, 1, 33); // ~30 checks/sec
        if (ret > 0 && (pfd.revents & POLLIN) && running_) {
            struct sockaddr_in clientAddr{};
            socklen_t addrLen = sizeof(clientAddr);
            int clientFd = accept(listenFd_,
                reinterpret_cast<struct sockaddr*>(&clientAddr), &addrLen);

            if (clientFd >= 0) {
                HandleNewConnection(clientFd);
            }
        }

        // Check for a new frame and broadcast to stream clients
        bool hasFrame = false;
        {
            std::lock_guard<std::mutex> lk(frameMutex_);
            if (frameReady_) {
                sendBuffer.swap(bmpBuffer_);
                frameReady_ = false;
                hasFrame = true;
            }
        }

        if (hasFrame) {
            BroadcastFrame(sendBuffer);
        }
    }
}

void LivePreviewModule::HandleNewConnection(int fd) {
    // Read the HTTP request (blocking with short timeout)
    char buf[2048];
    struct pollfd pfd{};
    pfd.fd = fd;
    pfd.events = POLLIN;

    int ret = poll(&pfd, 1, 1000); // 1s timeout for request
    if (ret <= 0) {
        close(fd);
        return;
    }

    ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
    if (n <= 0) {
        close(fd);
        return;
    }
    buf[n] = '\0';

    std::string path;
    if (!ParseHTTPRequest(buf, static_cast<size_t>(n), path)) {
        close(fd);
        return;
    }

    if (path == "/stream") {
        SetNoDelay(fd);
        // Blocking sends with 500ms timeout (safety net for slow clients)
        struct timeval tv{};
        tv.tv_usec = 500000;
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        SendStreamHeader(fd);
        {
            std::lock_guard<std::mutex> lk(clientsMutex_);
            streamClients_.push_back(fd);
        }
        clientCount_.fetch_add(1, std::memory_order_relaxed);
    } else {
        // Serve HTML page for any other path
        SendHTMLPage(fd);
        close(fd);
    }
}

void LivePreviewModule::SendHTMLPage(int fd) {
    size_t bodyLen = std::strlen(kHTMLPage);
    char header[256];
    int hLen = std::snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        bodyLen);

    send(fd, header, static_cast<size_t>(hLen), MSG_NOSIGNAL);
    send(fd, kHTMLPage, bodyLen, MSG_NOSIGNAL);
}

void LivePreviewModule::SendStreamHeader(int fd) {
    const char* hdr =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
        "Cache-Control: no-cache, no-store\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    send(fd, hdr, std::strlen(hdr), MSG_NOSIGNAL);
}

void LivePreviewModule::BroadcastFrame(const std::vector<uint8_t>& frame) {
    if (frame.empty()) return;

    // Build multipart frame header
    char partHeader[256];
    int phLen = std::snprintf(partHeader, sizeof(partHeader),
        "--frame\r\n"
        "Content-Type: image/bmp\r\n"
        "Content-Length: %zu\r\n"
        "\r\n",
        frame.size());

    std::lock_guard<std::mutex> lk(clientsMutex_);

    auto it = streamClients_.begin();
    while (it != streamClients_.end()) {
        int fd = *it;

        bool ok = SendAll(fd, partHeader, static_cast<size_t>(phLen))
                && SendAll(fd, frame.data(), frame.size())
                && SendAll(fd, "\r\n", 2);

        if (!ok) {
            close(fd);
            it = streamClients_.erase(it);
            clientCount_.fetch_sub(1, std::memory_order_relaxed);
        } else {
            ++it;
        }
    }
}

uint32_t LivePreviewModule::GetClientCount() const {
    std::lock_guard<std::mutex> lk(clientsMutex_);
    return static_cast<uint32_t>(streamClients_.size());
}

// -- Console commands ---------------------------------------------------------

void LivePreviewModule::RegisterCommands() {
    auto* consoleModule = static_cast<ConsoleModule*>(
        kernel_->Services().Get<void>("console"));
    if (!consoleModule) return;

    auto& cmds = consoleModule->Commands();

    cmds.Register({"preview.status", "preview.status",
        "Show live preview server status",
        [](KoiloKernel& /*k*/, const std::vector<std::string>& /*args*/)
            -> ConsoleResult {
            auto* mod = LivePreviewModule::Instance();
            if (!mod) {
                return ConsoleResult::Ok("Live preview module not active.");
            }

            int port = cvar_previewPort.Get();
            std::ostringstream os;
            os << "Live Preview Status:\n"
               << "  URL:      http://0.0.0.0:" << port << "/\n"
               << "  Clients:  " << mod->GetClientCount() << "\n"
               << "  Frames:   " << mod->GetFrameCount() << "\n"
               << "  Max FPS:  " << cvar_previewFps.Get() << "\n"
               << "  Running:  "
               << (mod->running_.load() ? "yes" : "no");
            return ConsoleResult::Ok(os.str());
        },
        nullptr
    });

    cmds.Register({"preview.url", "preview.url", "Print the preview URL",
        [](KoiloKernel& /*k*/, const std::vector<std::string>& /*args*/)
            -> ConsoleResult {
            int port = cvar_previewPort.Get();
            return ConsoleResult::Ok(
                "http://0.0.0.0:" + std::to_string(port) + "/");
        },
        nullptr
    });
}

} // namespace koilo

#endif // KL_HAVE_LIVE_PREVIEW
