# ESP32 Cooling Circuit

An ESP32 board (ESPertino-32) that reads temperature from a DHT11 sensor and switches a 12V fan on and off through a 5V relay module. A push button gives manual input.

![Wiring diagram](ESP32_cooling_circuit_enhanced.png)

## Parts

- ESP32 board (ESPertino-32)
- 5V relay module (SRD-05VDC-SL-C, high/low level trigger)
- 12V DC fan
- 12V DC adapter
- DHT11 temperature sensor
- Push button
- Breadboard and jumper wires

## Wiring

| Wire colour | Connection |
|---|---|
| Red | ESP32 5V to the relay module. The 12V adapter's positive side goes to the fan through the relay. |
| Orange | ESP32 3.3V to the breadboard power rail and the DHT11 supply pin. |
| Green | GPIO 27 to the relay IN pin. |
| Purple | GPIO 16 to the push button. |
| Blue | GPIO 4 to the DHT11 data pin. |
| Black | Common ground: ESP32, breadboard, sensor, relay and the 12V adapter's negative side. |

## How it works

1. The ESP32 reads the DHT11 on GPIO 4.
2. When the temperature goes above a threshold, the ESP32 sets GPIO 27 to trigger the relay. The relay closes the 12V circuit and the fan runs.
3. When the temperature drops back, the relay opens and the fan stops.
4. The push button on GPIO 16 can be used for a manual override or mode toggle. Its behaviour is defined in your code.

## Notes

- The fan runs on the separate 12V supply. Only the relay coil and signal connect to the ESP32 side.
- All grounds must be joined, or the relay and sensor won't work reliably.
- Check whether your relay is active-high or active-low (the jumper on the module sets this) and match it in your code.
- The diagram shows the wiring only. The threshold values and button behaviour are up to your firmware.
