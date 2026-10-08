# Hardware safety

Coffee Sentinel is designed to be **harmless to the coffee machine, to the
building's wiring and to the person building it**. These rules are not
negotiable and apply to every contribution.

## The coffee machine is never opened

* The device **observes the machine's READY LED optically**. A phototransistor
  is placed on the outside of the machine, in front of the LED, held with
  tape or a small 3D-printed cap.
* Nothing is soldered to, plugged into or drilled on the machine.
* The machine's electronics, wiring, thermostats, heater and pump are
  **out of scope**, including "just tapping the LED wires".
* Removing the device leaves the machine exactly as it was.

## No mains voltage

* The project works only with **3.3 V / 5 V DC** from a USB supply or a
  single Li-ion/LiPo cell.
* There is no relay, no triac and no connection to 127/220 V anywhere in the
  design, now or on the roadmap. Features that would need mains access
  (e.g. "turn the machine on remotely") are rejected by design.

## Low-voltage good practice

* Keep the phototransistor circuit on **3.3 V**; the ESP32 ADC pins are
  **not** 5 V tolerant.
* GPIOs source about 12 mA comfortably (40 mA absolute max). Buzzers that draw
  more need an NPN driver (see [wiring.md](wiring.md)).
* Do not power the board from USB and VIN at the same time.
* With a battery, always use a cell with a protection circuit or a module
  that includes one (DW01 + FS8205 on the common TP4056 boards), and never
  leave a bare cell charging unattended.

## Thermal and physical

* Mount the sensor away from the hot plate and steam vents; the sensor and the
  tape are rated for room temperature.
* Keep the device, cable and battery clear of the carafe and the water
  reservoir.

## Reporting

If you spot something in the hardware docs or the firmware that could lead to
an unsafe build, open an issue tagged `safety` before anything else.
