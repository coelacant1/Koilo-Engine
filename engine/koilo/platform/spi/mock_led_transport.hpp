// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file mock_led_transport.hpp
 * @brief Mock ILEDTransport for unit testing.
 *
 * Records all data sent through the transport and provides inspection
 * methods for test assertions. No platform dependencies.
 *
 * @date 04/06/2026
 * @author Coela Can't
 */

#pragma once

#include <koilo/systems/display/led/iled_transport.hpp>
#include <cstring>
#include <vector>

namespace koilo {

/**
 * @class MockLEDTransport
 * @brief Test double for ILEDTransport that records all transfers in memory.
 */
class MockLEDTransport : public ILEDTransport {
public:
    MockLEDTransport()
        : initialized_(false),
          ready_(true),
          failNextSend_(false),
          stats_() {}

    bool Initialize(const LEDTransportConfig& config) override {
        if (initialized_) { return false; }
        config_ = config;
        stats_ = LEDTransportStats();
        initialized_ = true;
        return true;
    }

    void Shutdown() override {
        initialized_ = false;
        sentBuffers_.clear();
    }

    bool IsInitialized() const override { return initialized_; }

    bool Send(const uint8_t* data, size_t len) override {
        if (!initialized_) {
            ++stats_.errors;
            return false;
        }
        if (!ready_) {
            ++stats_.framesDropped;
            return false;
        }
        if (failNextSend_) {
            failNextSend_ = false;
            ++stats_.errors;
            return false;
        }
        sentBuffers_.emplace_back(data, data + len);
        ++stats_.framesSent;
        stats_.bytesTransferred += len;
        stats_.lastTransferMs = 0.1f;
        return true;
    }

    bool IsReady() const override { return initialized_ && ready_; }

    LEDTransportStats GetStats() const override { return stats_; }

    // -- Test helpers --------------------------------------------------------

    /// @brief Number of successful Send() calls.
    size_t GetSendCount() const { return sentBuffers_.size(); }

    /// @brief Get the data from the Nth send call.
    const std::vector<uint8_t>& GetSentBuffer(size_t index) const {
        return sentBuffers_.at(index);
    }

    /// @brief Get the most recently sent buffer.
    const std::vector<uint8_t>& GetLastSentBuffer() const {
        return sentBuffers_.back();
    }

    /// @brief Force the next Send() call to fail.
    void SetFailNextSend(bool fail) { failNextSend_ = fail; }

    /// @brief Control the IsReady() return value.
    void SetReady(bool ready) { ready_ = ready; }

    /// @brief Get the config that was passed to Initialize().
    const LEDTransportConfig& GetConfig() const { return config_; }

    /// @brief Clear all recorded buffers (keeps initialized state).
    void ClearHistory() { sentBuffers_.clear(); }

private:
    bool initialized_;
    bool ready_;
    bool failNextSend_;
    LEDTransportConfig config_;
    LEDTransportStats stats_;
    std::vector<std::vector<uint8_t>> sentBuffers_;
};

} // namespace koilo
