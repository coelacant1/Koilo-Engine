// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file led_volume_hub75.ino
 * @brief Teensy 4.1 USB bulk firmware for Koilo Engine LED volume output
 *        using two 64x32 HUB75 panels driven via SmartMatrix (SmartLED Shield V5).
 *
 * Receives packed RGB LED data from a Raspberry Pi over USB (CDC ACM bulk
 * endpoints via SerialUSB1), validates CRC-CCITT, double-buffers the data,
 * and outputs to two stacked HUB75 panels (effective resolution 64x64) via
 * the SmartMatrix library.
 *
 * USB_DUAL_SERIAL mode provides two CDC ACM interfaces:
 *   - Serial   (/dev/ttyACM0) : debug output and status reporting
 *   - SerialUSB1 (/dev/ttyACM1) : LED frame data (binary bulk transfers)
 *
 * USB High-Speed (480 Mbps) gives 10-20+ MB/s practical throughput,
 * exceeding the ~1.5 MB/s needed for 4096 LEDs at 120 fps.
 * No SPI, no DMA conflicts with SmartMatrix.
 *
 * Panel layout (two 64x32 panels stacked vertically):
 *
 *   +-------------------+
 *   | Panel 1 (top)     |  rows 0-31
 *   | 64x32             |
 *   +-------------------+
 *   | Panel 2 (bottom)  |  rows 32-63
 *   | 64x32             |
 *   +-------------------+
 *
 * The engine sends 64x64 = 4096 pixels as a linear RGB888 stream.
 * Pixel [i] maps to x = i % 64, y = i / 64 (row-major, top-left origin).
 *
 * Wire protocol matches engine/koilo/systems/display/led/led_frame_encoder.hpp.
 *
 * @date 04/07/2026
 * @author Coela Can't
 */

#include <MatrixHardware_Teensy4_ShieldV5.h>
#include <SmartMatrix.h>

// ---------------------------------------------------------------------------
// HUB75 panel configuration
// ---------------------------------------------------------------------------

/// Panel resolution (two 64x32 stacked = 64x64).
static constexpr uint16_t PANEL_WIDTH  = 64;
static constexpr uint16_t PANEL_HEIGHT = 64;
static constexpr uint16_t TOTAL_PIXELS = PANEL_WIDTH * PANEL_HEIGHT;

/// SmartMatrix configuration.
static constexpr uint8_t  kRefreshDepth      = 36;
static constexpr uint8_t  kDmaBufferRows     = 4;
static constexpr uint8_t  kPanelType         = SM_PANELTYPE_HUB75_32ROW_MOD16SCAN;
static constexpr uint32_t kMatrixOptions      = SM_HUB75_OPTIONS_NONE;
static constexpr uint8_t  kBackgroundOptions  = SM_BACKGROUND_OPTIONS_NONE;

#define COLOR_DEPTH 24

SMARTMATRIX_ALLOCATE_BUFFERS(matrix, PANEL_WIDTH, PANEL_HEIGHT,
                             kRefreshDepth, kDmaBufferRows,
                             kPanelType, kMatrixOptions);

SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(bgLayer, PANEL_WIDTH, PANEL_HEIGHT,
                                      COLOR_DEPTH, kBackgroundOptions);

// ---------------------------------------------------------------------------
// General configuration
// ---------------------------------------------------------------------------

/// Maximum number of LEDs the protocol can handle.
static constexpr uint16_t MAX_LEDS = 5000;

/// On-board status LED.
static constexpr uint8_t STATUS_LED = 13;

/// Default brightness (0-255). Overridden by engine CVar led.brightness.
static constexpr uint8_t DEFAULT_BRIGHTNESS = 255;

// ---------------------------------------------------------------------------
// Wire protocol constants (must match led_frame_encoder.hpp)
// ---------------------------------------------------------------------------

static constexpr uint8_t  SYNC_BYTE_0  = 0xAA;
static constexpr uint8_t  SYNC_BYTE_1  = 0x55;
static constexpr size_t   HEADER_SIZE  = 6;   // SYNC(2) + SEQ(1) + SLOT(1) + COUNT(2)
static constexpr size_t   CRC_SIZE     = 2;
static constexpr uint16_t MAX_LED_COUNT = 8192;

/// Maximum frame size: header + 3 bytes per LED + CRC.
static constexpr size_t MAX_FRAME_SIZE = HEADER_SIZE + (MAX_LEDS * 3) + CRC_SIZE;

// ---------------------------------------------------------------------------
// State machine
// ---------------------------------------------------------------------------

enum class State : uint8_t {
    SYNC_0,
    SYNC_1,
    HEADER,
    PAYLOAD,
    CRC_LO,
    CRC_HI,
    VALIDATING
};

// ---------------------------------------------------------------------------
// Buffers
// ---------------------------------------------------------------------------

/// Double-buffered LED data. Each slot holds MAX_LEDS * 3 bytes of RGB888.
static uint8_t ledBuffer[2][MAX_LEDS * 3];

/// Which buffer slot is currently being displayed (0 or 1).
static volatile uint8_t displaySlot = 0;

/// Incoming frame receive buffer (raw bytes from USB).
static uint8_t rxBuf[MAX_FRAME_SIZE];
static size_t  rxPos = 0;

/// Parsed header fields.
static uint8_t  rxSeq  = 0;
static uint8_t  rxSlot = 0;
static uint16_t rxCount = 0;
static uint16_t rxCrcReceived = 0;

/// Active LED count from the last valid frame.
static uint16_t activeLedCount = 0;

/// Statistics.
static uint32_t framesReceived = 0;
static uint32_t crcErrors = 0;
static uint8_t  lastAckedSeq = 0;
static uint32_t totalBytesRead = 0;
static uint32_t syncHits = 0;

/// Parser state machine.
static State state = State::SYNC_0;
static size_t headerBytesRead = 0;
static size_t payloadBytesRead = 0;

// ---------------------------------------------------------------------------
// CRC-CCITT (reflected polynomial 0x8408, initial 0xFFFF)
// ---------------------------------------------------------------------------

static uint16_t computeCRC16(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= static_cast<uint16_t>(data[i]);
        for (int bit = 0; bit < 8; ++bit) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0x8408;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

// ---------------------------------------------------------------------------
// Frame parser (byte-at-a-time state machine)
// ---------------------------------------------------------------------------

static void resetParser() {
    state = State::SYNC_0;
    rxPos = 0;
    headerBytesRead = 0;
    payloadBytesRead = 0;
}

static void processByte(uint8_t b) {
    switch (state) {
        case State::SYNC_0:
            if (b == SYNC_BYTE_0) {
                state = State::SYNC_1;
                rxBuf[0] = b;
                rxPos = 1;
            }
            break;

        case State::SYNC_1:
            if (b == SYNC_BYTE_1) {
                syncHits++;
                rxBuf[1] = b;
                rxPos = 2;
                state = State::HEADER;
                headerBytesRead = 0;
            } else {
                resetParser();
            }
            break;

        case State::HEADER:
            rxBuf[rxPos++] = b;
            headerBytesRead++;
            if (headerBytesRead == 4) {
                rxSeq   = rxBuf[2];
                rxSlot  = rxBuf[3];
                rxCount = static_cast<uint16_t>(rxBuf[4]) |
                          (static_cast<uint16_t>(rxBuf[5]) << 8);

                if (rxCount == 0 || rxCount > MAX_LED_COUNT || rxCount > MAX_LEDS) {
                    resetParser();
                    break;
                }

                state = State::PAYLOAD;
                payloadBytesRead = 0;
            }
            break;

        case State::PAYLOAD:
            rxBuf[rxPos++] = b;
            payloadBytesRead++;
            if (payloadBytesRead == static_cast<size_t>(rxCount) * 3) {
                state = State::CRC_LO;
            }
            break;

        case State::CRC_LO:
            rxCrcReceived = b;
            state = State::CRC_HI;
            break;

        case State::CRC_HI:
            rxCrcReceived |= static_cast<uint16_t>(b) << 8;
            state = State::VALIDATING;
            break;

        case State::VALIDATING:
            break;
    }
}

// ---------------------------------------------------------------------------
// Frame validation and buffer swap
// ---------------------------------------------------------------------------

static void validateAndSwap() {
    const size_t crcDataLen = 4 + static_cast<size_t>(rxCount) * 3;
    const uint16_t computed = computeCRC16(rxBuf + 2, crcDataLen);

    if (computed != rxCrcReceived) {
        crcErrors++;
        resetParser();
        return;
    }

    const uint8_t targetSlot = rxSlot & 1;
    const size_t payloadBytes = static_cast<size_t>(rxCount) * 3;
    memcpy(ledBuffer[targetSlot], rxBuf + HEADER_SIZE, payloadBytes);

    displaySlot = targetSlot;
    activeLedCount = rxCount;
    framesReceived++;
    lastAckedSeq = rxSeq;

    resetParser();
}

// ---------------------------------------------------------------------------
// HUB75 panel output
// ---------------------------------------------------------------------------

static void outputToHUB75() {
    const uint8_t* buf = ledBuffer[displaySlot];

    const uint16_t count = (activeLedCount < TOTAL_PIXELS)
                           ? activeLedCount : TOTAL_PIXELS;

    for (uint16_t i = 0; i < count; ++i) {
        const size_t base = static_cast<size_t>(i) * 3;
        const uint8_t r = buf[base + 0];
        const uint8_t g = buf[base + 1];
        const uint8_t b = buf[base + 2];

        const uint16_t x = i % PANEL_WIDTH;
        const uint16_t y = i / PANEL_WIDTH;

        bgLayer.drawPixel(x, y, {r, g, b});
    }

    // Black-fill any remaining pixels
    for (uint16_t i = count; i < TOTAL_PIXELS; ++i) {
        const uint16_t x = i % PANEL_WIDTH;
        const uint16_t y = i / PANEL_WIDTH;
        bgLayer.drawPixel(x, y, {0, 0, 0});
    }

    bgLayer.swapBuffers(false);
}

// ---------------------------------------------------------------------------
// Status LED
// ---------------------------------------------------------------------------

static uint32_t lastBlinkMs = 0;
static bool ledState = false;

static void updateStatusLED() {
    const uint32_t now = millis();
    uint32_t interval;

    if (crcErrors > 0 && framesReceived == 0) {
        interval = 1000;
    } else if (framesReceived > 0) {
        interval = 100;
    } else {
        digitalWrite(STATUS_LED, HIGH);
        return;
    }

    if (now - lastBlinkMs >= interval) {
        ledState = !ledState;
        digitalWrite(STATUS_LED, ledState ? HIGH : LOW);
        lastBlinkMs = now;
    }
}

// ---------------------------------------------------------------------------
// Arduino entry points
// ---------------------------------------------------------------------------

void setup() {
    pinMode(STATUS_LED, OUTPUT);
    digitalWrite(STATUS_LED, HIGH);

    Serial.begin(115200);
    SerialUSB1.begin(0);  // Baud rate ignored for USB CDC ACM

    Serial.println("[HUB75] Teensy 4.1 USB LED Volume starting...");

    // Clear both LED buffers.
    memset(ledBuffer[0], 0, sizeof(ledBuffer[0]));
    memset(ledBuffer[1], 0, sizeof(ledBuffer[1]));

    // Initialize SmartMatrix HUB75.
    matrix.addLayer(&bgLayer);
    matrix.begin();
    matrix.setRefreshRate(240);
    matrix.setBrightness(DEFAULT_BRIGHTNESS);

    // First swapBuffers clears the display.
    bgLayer.swapBuffers();

    Serial.print("[HUB75] Ready. Panel=");
    Serial.print(PANEL_WIDTH);
    Serial.print("x");
    Serial.print(PANEL_HEIGHT);
    Serial.print(" (");
    Serial.print(TOTAL_PIXELS);
    Serial.println(" pixels). USB data on SerialUSB1.");
}

void loop() {
    // Read all available bytes from USB data port.
    while (SerialUSB1.available()) {
        uint8_t b = SerialUSB1.read();
        totalBytesRead++;
        processByte(b);

        if (state == State::VALIDATING) {
            validateAndSwap();
            outputToHUB75();
        }
    }

    updateStatusLED();

    // Periodic debug stats (every 5 seconds).
    static uint32_t lastStatMs = 0;
    const uint32_t now = millis();
    if (now - lastStatMs >= 5000) {
        Serial.print("[HUB75] frames=");
        Serial.print(framesReceived);
        Serial.print(" crc_errs=");
        Serial.print(crcErrors);
        Serial.print(" bytes=");
        Serial.print(totalBytesRead);
        Serial.print(" syncs=");
        Serial.print(syncHits);
        Serial.print(" leds=");
        Serial.print(activeLedCount);
        Serial.print(" seq=");
        Serial.println(lastAckedSeq);
        lastStatMs = now;
    }
}
