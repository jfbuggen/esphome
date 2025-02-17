# ESPHome AHT21 temperature and humidity i2c meter

This repository adds support for AHT21 devices to [ESPHome](https://esphome.io).

## Why not use the standard AHT10 component from ESPHome?

As part of the ESPHome platform, the [AHT10 component](https://esphome.io/components/sensor/aht10.html) is said to be compatible with AHT20. A "variant" can be configured for this.
However, even if the code seems to work as well for AHT21 (setting "variant" to AHT20), I realised that the sample code provided by the manufacturer [(link to .rar file)](http://aosong.com/userfiles/files/software/AHT20-21%20DEMO%20V1_3(1).rar) differs, notably in the initalisation sequence.
I thought I would adapt the AHT10 code to add a AHT21 variant.

But then I also realised that the device is heating rapidly, which corrupts the temperature measure. 
I wanted to power the AHT21 module via a GPIO only during measurement.

Adapting the existing component was not easy, because AHT21 requires at least 500ms after power-up.
This is not ok for a polling component, it would take too long as part of the "update".

This new component behaves differently:
- A state machine splits the processing in small bits, ensuring that the component does not hold control for too long, even if the full process chain takes typically 720ms (!)
- At regular intervals, the AHT21 is powered up, initialised, and a reading is made, then put back to sleep

I have to say that I'm still moderately satisfied with the result. The code works fine, but despite scan intervals of 5 minutes (started initially with 1 minute), the AHT21 remains very sensitive to temperature if it is put in a printed case. I'll probably turn to other sensors (bme280?), so there is little chance that I implement soon the items under "to do".

## Configuration

This component can be easily added as an [external component](https://esphome.io/components/external_components.html) to ESPHome.

```yaml
external_components:
  - source: https://github.com/jfbuggen/esphome/

i2c:
  frequency: 10kHz    # low frequency to limit heating
  scan: false         # disable scanning as un-powered AHT21 causes issues on I2C bus...

sensor:
  - platform: aht21
    gpio: GPIO14      # pin that is used to power the AHT21
    temperature:
      name: "Living Room Temperature"
    humidity:
      name: "Living Room Humidity"
```

## Configuration Variables

### Sensor

- **gpio** (Required, [pin](https://esphome.io/guides/configuration-types.html#pin)): GPIO output pin used to power the AHT21.
- **temperature** (Required): The information for the temperature sensor.
  * All options from [Sensor](https://esphome.io/components/sensor/#config-sensor).
- **humidity** (Required): The information for the humidity sensor.
  * All options from [Sensor](https://esphome.io/components/sensor/#config-sensor).
---

## To Do
- Add configurable measure interval
- Make gpio optional. If not present, skip power-up/down and replace by AHT21 soft reset
- Allow disabling/ignoring CRC checks via a yaml option (even if I never had CRC issues)
- Add AHT10 and AHT21 variants
