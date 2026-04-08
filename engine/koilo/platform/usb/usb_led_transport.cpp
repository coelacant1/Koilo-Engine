// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file usb_led_transport.cpp
 * @brief USB serial (CDC ACM) ILEDTransport implementation.
 *
 * Uses POSIX termios for raw binary I/O to a USB CDC ACM device.
 * All error paths report via std::cerr following the existing
 * display-backend convention.
 *
 * @date 04/07/2026
 * @author Coela Can't
 */

#ifdef KL_HAVE_USB_SERIAL

#include <koilo/platform/usb/usb_led_transport.hpp>

#include <cerrno>
#include <cstring>
#include <iostream>

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>

namespace koilo {

// ---- helpers ---------------------------------------------------------------

namespace {

/// @brief Measure wall-clock time of a transfer in milliseconds.
struct TransferTimer {
    struct timespec start;
    TransferTimer() { clock_gettime(CLOCK_MONOTONIC, &start); }
    float ElapsedMs() const {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        double sec = static_cast<double>(now.tv_sec - start.tv_sec);
        double ns  = static_cast<double>(now.tv_nsec - start.tv_nsec);
        return static_cast<float>((sec * 1000.0) + (ns / 1e6));
    }
};

} // anonymous namespace

// ---- lifecycle -------------------------------------------------------------

USBLEDTransport::USBLEDTransport()
    : fd_(-1),
      config_(),
      stats_() {}

USBLEDTransport::~USBLEDTransport() {
    if (fd_ >= 0) {
        Shutdown();
    }
}

bool USBLEDTransport::Initialize(const LEDTransportConfig& config) {
    if (fd_ >= 0) {
        std::cerr << "[USBLEDTransport] Already initialized" << std::endl;
        return false;
    }

    config_ = config;
    stats_ = LEDTransportStats();

    // Open with O_NOCTTY (no controlling terminal) and O_NONBLOCK
    // to avoid blocking on DCD. We clear O_NONBLOCK after setup.
    fd_ = ::open(config_.devicePath.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0) {
        std::cerr << "[USBLEDTransport] Failed to open "
                  << config_.devicePath << ": "
                  << std::strerror(errno) << std::endl;
        return false;
    }

    if (!ConfigurePort()) {
        std::cerr << "[USBLEDTransport] Port configuration failed" << std::endl;
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    // Switch to blocking mode for writes.
    int flags = ::fcntl(fd_, F_GETFL);
    if (flags >= 0) {
        ::fcntl(fd_, F_SETFL, flags & ~O_NONBLOCK);
    }

    std::cerr << "[USBLEDTransport] Opened " << config_.devicePath
              << " (USB CDC ACM bulk)" << std::endl;
    return true;
}

void USBLEDTransport::Shutdown() {
    if (fd_ >= 0) {
        // Drain any pending output before closing.
        ::tcdrain(fd_);
        ::close(fd_);
        fd_ = -1;
        std::cerr << "[USBLEDTransport] Shutdown" << std::endl;
    }
}

bool USBLEDTransport::IsInitialized() const {
    return fd_ >= 0;
}

// ---- data transfer ---------------------------------------------------------

bool USBLEDTransport::Send(const uint8_t* data, size_t len) {
    if (fd_ < 0) {
        ++stats_.errors;
        return false;
    }
    if (data == nullptr || len == 0) {
        return true;
    }

    TransferTimer timer;

    // Handle partial writes (USB serial may fragment large transfers).
    size_t totalWritten = 0;
    while (totalWritten < len) {
        ssize_t n = ::write(fd_, data + totalWritten, len - totalWritten);
        if (n > 0) {
            totalWritten += static_cast<size_t>(n);
        } else if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            ++stats_.errors;
            std::cerr << "[USBLEDTransport] Write failed: "
                      << std::strerror(errno) << std::endl;
            return false;
        } else {
            // n == 0: device not accepting data
            ++stats_.errors;
            return false;
        }
    }

    stats_.lastTransferMs = timer.ElapsedMs();
    ++stats_.framesSent;
    stats_.bytesTransferred += len;
    return true;
}

bool USBLEDTransport::IsReady() const {
    return fd_ >= 0;
}

LEDTransportStats USBLEDTransport::GetStats() const {
    return stats_;
}

// ---- internal --------------------------------------------------------------

bool USBLEDTransport::ConfigurePort() {
    struct termios tio;
    if (::tcgetattr(fd_, &tio) < 0) {
        std::cerr << "[USBLEDTransport] tcgetattr failed: "
                  << std::strerror(errno) << std::endl;
        return false;
    }

    // Raw binary mode: no echo, no signal chars, no canonical processing.
    ::cfmakeraw(&tio);

    // Disable hardware and software flow control.
    tio.c_cflag &= ~static_cast<tcflag_t>(CRTSCTS);
    tio.c_cflag |= CLOCAL;
    tio.c_iflag &= ~static_cast<tcflag_t>(IXON | IXOFF | IXANY);

    // Non-blocking reads (we primarily write, reads are optional ACKs).
    tio.c_cc[VMIN]  = 0;
    tio.c_cc[VTIME] = 0;

    // Set a high baud rate. For USB CDC ACM the actual transfer speed is
    // governed by the USB bus (480 Mbps HS), but some drivers inspect the
    // baud setting for internal buffer management.
    ::cfsetispeed(&tio, B921600);
    ::cfsetospeed(&tio, B921600);

    if (::tcsetattr(fd_, TCSANOW, &tio) < 0) {
        std::cerr << "[USBLEDTransport] tcsetattr failed: "
                  << std::strerror(errno) << std::endl;
        return false;
    }

    // Flush any stale data in the serial buffers.
    ::tcflush(fd_, TCIOFLUSH);

    return true;
}

} // namespace koilo

#endif // KL_HAVE_USB_SERIAL
