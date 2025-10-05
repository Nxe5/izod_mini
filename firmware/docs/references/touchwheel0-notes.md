# TouchWheel0 Notes

We will prototype a capacitive touch wheel (TouchWheel0). Initial experiments can follow the provided CircuitPython example logic and then be ported to Arduino. As an alternative, the MPR121 capacitive controller can provide 8–12 electrodes for a segmented wheel.

## CircuitPython Concept (from example)
- Three capacitive inputs produce raw values
- Compute normalized percentages
- Map combinations of two active pads to a position on a virtual circle
- Provide `pos ∈ [0,1)` or `None`

## Arduino Port Plan
- If using native touch:
  - Use ESP32-S3 touch/cap sense pins (where available) or an external ADC approach
  - Sample three (or more) electrodes, apply thresholds
  - Replicate `wheel_pos(a,b,c)` math to compute angle/segment
- If using MPR121:
  - Map 8–12 electrodes around the wheel; use adjacent activation ratios to compute position
  - Library: Adafruit MPR121; I2C on SDA=3, SCL=4

## UI Mapping
- Rotation → scroll lists / scrub track
- Tap zones → select/back
- Long-press → context actions

## Next Steps
1. Breadboard 3‑pad wheel and log values while rotating
2. Implement position estimator and smoothing
3. Integrate with Input Task; send events via queue
