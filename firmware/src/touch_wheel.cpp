#include "touch_wheel.h"
#include <Wire.h>
#include <Adafruit_MPR121.h>
#include <math.h>

#ifndef I2C_SDA
#define I2C_SDA 3
#endif
#ifndef I2C_SCL
#define I2C_SCL 4
#endif
#ifndef MPR121_ADDR
#define MPR121_ADDR 0x5A
#endif

static Adafruit_MPR121 s_mpr;
static TaskHandle_t s_touchTask = NULL;
static volatile int s_accumDelta = 0;
static volatile bool s_connected = false;
static uint8_t s_touchThresh = 12;   // default reasonable
static uint8_t s_releaseThresh = 6;  // default reasonable
static volatile bool s_debugRaw = false;

static const int kElectrodes = 12;   // using 12 pads as a ring
static uint8_t s_order[12] = {0,1,2,3,4,5,6,7,8,9,10,11};
static uint8_t s_orderCount = 12;
static bool s_invert = false;
static float s_fracMove = 0.0f; // accumulate fractional motion into whole steps
static bool s_isActive = false;
static int s_activeFrames = 0;
static int s_quietFrames = 0;

static inline int wrapIndex(int idx, int n) {
    if (idx < 0) return idx + n;
    if (idx >= n) return idx - n;
    return idx;
}

static void touchTask(void* pv) {
    uint16_t lastTouched = 0;
    float lastPos = -1.0f;
    TickType_t lastTick = xTaskGetTickCount();
    uint32_t lastLogMs = 0;
    for(;;) {
        if (!s_connected) { vTaskDelay(pdMS_TO_TICKS(50)); continue; }
        uint16_t curTouched = s_mpr.touched();
        uint16_t pressedBits = (curTouched & ~lastTouched) & 0x0FFF;
        uint16_t releasedBits = (~curTouched & lastTouched) & 0x0FFF;

        // Vector-sum center of mass around the circle (robust with multi-touch)
        float sumX = 0.0f, sumY = 0.0f, totalMag = 0.0f;
        float maxStrength = 0.0f;
        const float kStrengthMin = 0.5f;
        for (int i = 0; i < s_orderCount; i++) {
            int e = s_order[i];
            uint16_t filtered = s_mpr.filteredData(e);
            uint16_t baseline = s_mpr.baselineData(e);
            float strength = (float)((int)baseline - (int)filtered);
            if (strength < kStrengthMin) strength = 0.0f;
            if (strength > 0.0f) {
                float ang = (2.0f * (float)M_PI * i) / (float)s_orderCount;
                sumX += cosf(ang) * strength;
                sumY += sinf(ang) * strength;
                totalMag += strength;
                if (strength > maxStrength) maxStrength = strength;
            }
        }
        // Fallback using inverse filtered if baseline is not useful
        if (totalMag <= 0.0f) {
            const float kFilteredMax = 32.0f;
            for (int i = 0; i < s_orderCount; i++) {
                int e = s_order[i];
                float strength = fmaxf(0.0f, kFilteredMax - (float)s_mpr.filteredData(e));
                if (strength > 1.0f) {
                    float ang = (2.0f * (float)M_PI * i) / (float)s_orderCount;
                    sumX += cosf(ang) * strength;
                    sumY += sinf(ang) * strength;
                    totalMag += strength;
                    if (strength > maxStrength) maxStrength = strength;
                }
            }
        }

        float activePos = -1.0f;
        if (totalMag > 0.0f) {
            float angle = atan2f(sumY, sumX); // -pi..pi
            if (angle < 0) angle += 2.0f * (float)M_PI; // 0..2pi
            // Map angle to [0..s_orderCount) position space
            activePos = angle * (float)s_orderCount / (2.0f * (float)M_PI);
            Serial.printf("TW: COM angle=%.2f deg pos=%.2f mag=%.2f\n", angle * 180.0f / (float)M_PI, activePos, totalMag);
        }

        // Gate on activity to prevent drift when not touched
        // Strict gating: only consider active when MPR121 reports any touched bit
        const float kMaxStrengthActive = 6.0f;
        const float kTotalMagActive   = 20.0f;
        bool touchedBits = (curTouched != 0);
        // Allow a small hysteresis: require touched bits OR very strong signal
        bool proposedActive = touchedBits || (maxStrength >= 10.0f);
        if (proposedActive) { s_activeFrames++; s_quietFrames = 0; } else { s_quietFrames++; s_activeFrames = 0; }
        bool prevActive = s_isActive;
        if (!s_isActive && s_activeFrames >= 2) s_isActive = true;           // debounce into active
        if (s_isActive && s_quietFrames >= 3) s_isActive = false;            // debounce into inactive
        if (prevActive != s_isActive) Serial.printf("TW: Active=%s (maxStr=%.1f totalMag=%.1f touched=%s)\n",
                                                   s_isActive?"YES":"NO", maxStrength, totalMag, touchedBits?"Y":"N");

        int stepDelta = 0;
        if (s_isActive && activePos >= 0 && lastPos >= 0) {
            // Compute position delta using interpolated positions
            float diff = activePos - lastPos;
            // Handle wrap-around on circular wheel
            if (diff > s_orderCount/2.0f) diff -= s_orderCount;
            if (diff < -s_orderCount/2.0f) diff += s_orderCount;
            if (s_invert) diff = -diff;
            // Accumulate fractional motion and emit integer steps when passing 0.5
            s_fracMove += diff;
            while (s_fracMove >= 0.5f) { s_accumDelta += 1; stepDelta += 1; s_fracMove -= 1.0f; }
            while (s_fracMove <= -0.5f) { s_accumDelta -= 1; stepDelta -= 1; s_fracMove += 1.0f; }
            Serial.printf("TW: Motion last=%.2f pos=%.2f diff=%.2f frac=%.2f stepDelta=%d accum=%d\n", lastPos, activePos, diff, s_fracMove, stepDelta, (int)s_accumDelta);
        }
        // Update last position or reset when inactive
        if (s_isActive && activePos >= 0) lastPos = activePos;
        else { lastPos = -1.0f; s_fracMove = 0.0f; }

        // Edge logs
        if (pressedBits) {
            for (int i = 0; i < s_orderCount; i++) if (pressedBits & (1 << s_order[i])) Serial.printf("TW: E%d pressed\n", s_order[i]);
        }
        if (releasedBits) {
            for (int i = 0; i < s_orderCount; i++) if (releasedBits & (1 << s_order[i])) Serial.printf("TW: E%d released\n", s_order[i]);
        }

        // Log state changes (rate-limited summary)
        uint32_t nowMs = millis();
        if ((curTouched != lastTouched || stepDelta != 0) && (nowMs - lastLogMs > 20)) {
            Serial.printf("TW: touch=0x%03X active=%.1f step=%d accum=%d\n", curTouched & 0x0FFF, activePos, stepDelta, (int)s_accumDelta);
            lastLogMs = nowMs;
        }

        // Optional raw dumps like Adafruit example
        if (s_debugRaw && (nowMs - lastLogMs > 40)) {
            for (uint8_t i = 0; i < s_orderCount; i++) Serial.printf("%5u ", s_mpr.baselineData(s_order[i]));
            Serial.println();
            for (uint8_t i = 0; i < s_orderCount; i++) Serial.printf("%5u ", s_mpr.filteredData(s_order[i]));
            Serial.println();
            lastLogMs = nowMs;
        }

        // Polling period ~8 ms for responsive scroll
        vTaskDelayUntil(&lastTick, pdMS_TO_TICKS(8));
        lastTick = xTaskGetTickCount();
        lastTouched = curTouched;
    }
}

bool touchWheelInit(uint8_t i2cAddress) {
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(400000);
    if (!s_mpr.begin(i2cAddress)) {
        s_connected = false;
        return false;
    }
    s_connected = true;
    // Configure global thresholds for all electrodes
    s_mpr.setThresholds(s_touchThresh, s_releaseThresh);
    if (!s_touchTask) xTaskCreatePinnedToCore(touchTask, "TouchWheel", 3072, NULL, 1, &s_touchTask, 1);
    return true;
}

bool touchWheelIsConnected() { return s_connected; }

int touchWheelGetAndClearDelta() {
    int d = s_accumDelta;
    s_accumDelta = 0;
    return d;
}

void touchWheelSetThresholds(uint8_t touchThresh, uint8_t releaseThresh) {
    s_touchThresh = touchThresh; s_releaseThresh = releaseThresh;
    if (!s_connected) return;
    s_mpr.setThresholds(s_touchThresh, s_releaseThresh);
}

void touchWheelGetThresholds(uint8_t* touchThresh, uint8_t* releaseThresh) {
    if (touchThresh) *touchThresh = s_touchThresh;
    if (releaseThresh) *releaseThresh = s_releaseThresh;
}

void touchWheelSetDebugRaw(bool enable) { s_debugRaw = enable; }
bool touchWheelGetDebugRaw() { return s_debugRaw; }

void touchWheelSetElectrodeOrder(const uint8_t* order, uint8_t count) {
    if (!order || count == 0 || count > 12) return;
    s_orderCount = count;
    for (uint8_t i = 0; i < count; i++) s_order[i] = order[i];
}

void touchWheelSetInvert(bool invert) { s_invert = invert; }


