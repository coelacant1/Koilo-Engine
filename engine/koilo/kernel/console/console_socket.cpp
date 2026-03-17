#include <koilo/kernel/console/console_socket.hpp>
#include <koilo/kernel/kernel.hpp>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    using socklen_t = int;
    #define CLOSE_SOCKET closesocket
    #define SOCKET_ERROR_CODE WSAGetLastError()
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #define CLOSE_SOCKET ::close
    #define SOCKET_ERROR_CODE errno
#endif

namespace koilo {

ConsoleSocket::ConsoleSocket(KoiloKernel& kernel, CommandRegistry& registry)
    : kernel_(kernel), registry_(registry) {
}

ConsoleSocket::~ConsoleSocket() {
    Stop();
}

bool ConsoleSocket::Start(uint16_t port) {
    if (running_.load()) return false;

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::fprintf(stderr, "[Console] WSAStartup failed\n");
        return false;
    }
#endif

    serverFd_ = static_cast<int>(socket(AF_INET, SOCK_STREAM, 0));
    if (serverFd_ < 0) {
        std::fprintf(stderr, "[Console] Failed to create socket: %d\n", SOCKET_ERROR_CODE);
        return false;
    }

    int opt = 1;
    setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
#ifdef SO_REUSEPORT
    setsockopt(serverFd_, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<const char*>(&opt), sizeof(opt));
#endif

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // localhost only
    addr.sin_port = htons(port);

    if (bind(serverFd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::fprintf(stderr, "[Console] Failed to bind port %u: %d\n", port, SOCKET_ERROR_CODE);
        CLOSE_SOCKET(serverFd_);
        serverFd_ = -1;
        return false;
    }

    if (listen(serverFd_, 4) < 0) {
        std::fprintf(stderr, "[Console] Failed to listen: %d\n", SOCKET_ERROR_CODE);
        CLOSE_SOCKET(serverFd_);
        serverFd_ = -1;
        return false;
    }

    port_ = port;
    running_.store(true);
    acceptThread_ = std::thread(&ConsoleSocket::AcceptLoop, this);

    std::fprintf(stderr, "[Console] Listening on 127.0.0.1:%u\n", port);
    return true;
}

void ConsoleSocket::Stop() {
    if (!running_.load()) return;
    running_.store(false);

    if (serverFd_ >= 0) {
        CLOSE_SOCKET(serverFd_);
        serverFd_ = -1;
    }

    if (acceptThread_.joinable()) {
        acceptThread_.join();
    }

    for (auto& t : clientThreads_) {
        if (t.joinable()) t.join();
    }
    clientThreads_.clear();
}

void ConsoleSocket::AcceptLoop() {
    while (running_.load()) {
        struct sockaddr_in clientAddr{};
        socklen_t addrLen = sizeof(clientAddr);

        int clientFd = static_cast<int>(
            accept(serverFd_, reinterpret_cast<struct sockaddr*>(&clientAddr), &addrLen));

        if (clientFd < 0) {
            if (running_.load()) {
                // Only log if we didn't intentionally close
            }
            continue;
        }

        std::fprintf(stderr, "[Console] Client connected\n");
        clientThreads_.emplace_back(&ConsoleSocket::HandleClient, this, clientFd);
    }
}

void ConsoleSocket::HandleClient(int clientFd) {
    ConsoleSession session(kernel_, registry_);
    session.SetOutputFormat(ConsoleOutputFormat::Json);

    // Send welcome
    const char* welcome = "{\"status\":\"connected\",\"engine\":\"koilo\",\"version\":\"0.3.2\"}\n";
    ::send(clientFd, welcome, static_cast<int>(std::strlen(welcome)), 0);

    char buf[4096];
    std::string lineBuffer;

    while (running_.load()) {
        int n = static_cast<int>(recv(clientFd, buf, sizeof(buf) - 1, 0));
        if (n <= 0) break;

        buf[n] = '\0';
        lineBuffer += buf;

        // Process complete lines
        size_t pos;
        while ((pos = lineBuffer.find('\n')) != std::string::npos) {
            std::string line = lineBuffer.substr(0, pos);
            lineBuffer.erase(0, pos + 1);

            // Trim \r
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;

            auto result = session.Execute(line);

            // Format response as JSON
            std::string response;
            if (!result.json.empty()) {
                response = "{\"status\":\"" +
                    std::string(result.ok() ? "ok" : "error") +
                    "\",\"data\":" + result.json + "}\n";
            } else {
                // Escape text for JSON
                std::string escaped;
                for (char c : result.text) {
                    if (c == '"') escaped += "\\\"";
                    else if (c == '\\') escaped += "\\\\";
                    else if (c == '\n') escaped += "\\n";
                    else if (c == '\t') escaped += "\\t";
                    else escaped += c;
                }
                response = "{\"status\":\"" +
                    std::string(result.ok() ? "ok" : "error") +
                    "\",\"text\":\"" + escaped + "\"}\n";
            }

            ::send(clientFd, response.c_str(), static_cast<int>(response.size()), 0);
        }
    }

    CLOSE_SOCKET(clientFd);
    std::fprintf(stderr, "[Console] Client disconnected\n");
}

} // namespace koilo
