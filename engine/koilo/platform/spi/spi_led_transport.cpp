// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file spi_led_transport.cpp
 * @brief Linux spidev ILEDTransport implementation.
 *
 * Uses ioctl SPI_IOC_MESSAGE for full-duplex transfers. All error paths
 * report via std::cerr following the existing display-backend convention.
 *
 * @date 04/06/2026
 * @author Coela Can't
 */

#ifdef KL_HAVE_SPIDEV

#include <koilo/platform/spi/spi_led_transport.hpp>

#include <cerrno>
#include <cstring>
#include <iostream>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

namespace koilo {

// ---- helpers ---------------------------------------------------------------

namespace {

/// @brief Measure wall-clock time of an SPI transfer in milliseconds.
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

SPILEDTransport::SPILEDTransport()
    : fd_(-1),
      config_(),
      stats_() {}

SPILEDTransport::~SPILEDTransport() {
    if (fd_ >= 0) {
        Shutdown();
    }
}

bool SPILEDTransport::Initialize(const LEDTransportConfig& config) {
    if (fd_ >= 0) {
        std::cerr << "[SPILEDTransport] Already initialized" << std::endl;
        return false;
    }

    config_ = config;
    stats_ = LEDTransportStats();

    fd_ = ::open(config_.devicePath.c_str(), O_RDWR);
    if (fd_ < 0) {
        std::cerr << "[SPILEDTransport] Failed to open "
                  << config_.devicePath << ": "
                  << std::strerror(errno) << std::endl;
        return false;
    }

    if (!ConfigureBus()) {
        std::cerr << "[SPILEDTransport] Bus configuration failed" << std::endl;
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    std::cerr << "[SPILEDTransport] Opened " << config_.devicePath
              << " @ " << config_.clockHz << " Hz, mode "
              << static_cast<int>(config_.mode) << std::endl;
    return true;
}

void SPILEDTransport::Shutdown() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
        std::cerr << "[SPILEDTransport] Shutdown" << std::endl;
    }
}

bool SPILEDTransport::IsInitialized() const {
    return fd_ >= 0;
}

// ---- data transfer ---------------------------------------------------------

bool SPILEDTransport::Send(const uint8_t* data, size_t len) {
    if (fd_ < 0) {
        ++stats_.errors;
        return false;
    }
    if (data == nullptr || len == 0) {
        return true;
    }

    struct spi_ioc_transfer xfer;
    std::memset(&xfer, 0, sizeof(xfer));
    xfer.tx_buf        = reinterpret_cast<unsigned long>(data);
    xfer.rx_buf        = 0;
    xfer.len           = static_cast<uint32_t>(len);
    xfer.speed_hz      = config_.clockHz;
    xfer.bits_per_word = config_.bitsPerWord;
    xfer.delay_usecs   = 0;

    TransferTimer timer;
    int ret = ::ioctl(fd_, SPI_IOC_MESSAGE(1), &xfer);
    stats_.lastTransferMs = timer.ElapsedMs();

    if (ret < 0) {
        ++stats_.errors;
        std::cerr << "[SPILEDTransport] Transfer failed: "
                  << std::strerror(errno) << std::endl;
        return false;
    }

    ++stats_.framesSent;
    stats_.bytesTransferred += len;
    return true;
}

bool SPILEDTransport::IsReady() const {
    return fd_ >= 0;
}

LEDTransportStats SPILEDTransport::GetStats() const {
    return stats_;
}

// ---- internal --------------------------------------------------------------

bool SPILEDTransport::ConfigureBus() {
    uint8_t mode = config_.mode;
    if (::ioctl(fd_, SPI_IOC_WR_MODE, &mode) < 0) {
        std::cerr << "[SPILEDTransport] SPI_IOC_WR_MODE failed: "
                  << std::strerror(errno) << std::endl;
        return false;
    }

    uint8_t bits = config_.bitsPerWord;
    if (::ioctl(fd_, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
        std::cerr << "[SPILEDTransport] SPI_IOC_WR_BITS_PER_WORD failed: "
                  << std::strerror(errno) << std::endl;
        return false;
    }

    uint32_t speed = config_.clockHz;
    if (::ioctl(fd_, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        std::cerr << "[SPILEDTransport] SPI_IOC_WR_MAX_SPEED_HZ failed: "
                  << std::strerror(errno) << std::endl;
        return false;
    }

    return true;
}

} // namespace koilo

#endif // KL_HAVE_SPIDEV
