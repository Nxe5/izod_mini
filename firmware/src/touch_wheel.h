#pragma once

#include <Arduino.h>

// MPR121-based 12-segment scroll wheel helper
// Initializes I2C + MPR121, runs a small poller task, and accumulates
// signed step deltas (+CW / -CCW) that the app can consume.

// Initialize the touch wheel. Returns true on success.
bool touchWheelInit(uint8_t i2cAddress = 0x5A);

// Whether the MPR121 was detected and initialized.
bool touchWheelIsConnected();

// Get accumulated scroll delta since last call and clear it.
// Positive = clockwise, Negative = counter-clockwise.
int touchWheelGetAndClearDelta();

// Optionally adjust thresholds.
void touchWheelSetThresholds(uint8_t touchThresh, uint8_t releaseThresh);

// Read back current thresholds
void touchWheelGetThresholds(uint8_t* touchThresh, uint8_t* releaseThresh);

// Enable/disable raw debug logging (baseline/filtered per electrode)
void touchWheelSetDebugRaw(bool enable);
bool touchWheelGetDebugRaw();

// Define the physical order of electrodes around the wheel.
// Example for straight mapping: order[12] = {0,1,2,3,4,5,6,7,8,9,10,11}
// Pass count=12. Call before init or anytime (takes effect next poll).
void touchWheelSetElectrodeOrder(const uint8_t* order, uint8_t count);

// Invert direction (swap CW/CCW) if rotation feels backwards
void touchWheelSetInvert(bool invert);


