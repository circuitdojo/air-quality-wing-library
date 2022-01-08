# Air Quality Wing Library! (Deprecated)

![Render](images/render.png)

The Air Quality Wing (Previously Particle Squared) is a circuit board that helps you monitor the air quality around you. This repository is the code for the Air Quality Wing which enables you to monitor humidity, temperature, air particulates, eC02 and TVOCs in one small package.

You can use it with an Adafruit Feather compatible board. [For more information click here.](https://docs.jaredwolff.com/air-quality-wing/index.html)

[To get yours go here.](https://www.jaredwolff.com/store/air-quality-wing/)

*Update 1/8/2022*: This sample is now deprecated. Please see the [Zephyr samples](https://github.com/circuitdojo/air-quality-wing-zephyr-demo) for the latest supported working code.

## Usage

Insert a Xenon, Boron or Argon into a Air Quality Wing. Create a new project and import the library. (Search for `AirQualityWing`)

Here's a base example. (Also available under `AirQualityWing/AirQualityWing.ino`)

```c++
/*
 * Project Air Quality Wing Library Example
 * Description: Basic example using the Air Quality Wing for Particle Mesh
 * Author: Jared Wolff (Circuit Dojo LLC)
 * Date: 10/27/2019
 * License: GNU GPLv3
 */

#include "AirQualityWing.h"
#include "board.h"

// Logger
SerialLogHandler logHandler(115200, LOG_LEVEL_ERROR, {
    { "app", LOG_LEVEL_TRACE }, // enable all app messages
});

// Forward declaration of event handler
void AirQualityWingEvent();

// AirQualityWing object
AirQualityWing AirQual = AirQualityWing();

// Handler is called in main loop.
// Ok to run Particle.Publish
void AirQualityWingEvent() {

  Log.trace("pub");

  // Publish event
  Particle.publish("blob", AirQual.toString(), PRIVATE, WITH_ACK);

}

// Cloud function for setting interval
int set_interval( String period ) {

  // Set the interval with the air quality code
  AirQual.setInterval((uint32_t)period.toInt());

  return -1;

}

// setup() runs once, when the device is first turned on.
void setup() {

  // Turn off the LED
  // RGB.control(true);
  // RGB.brightness(0);

  // Set up PC based UART (for debugging)
  Serial.blockOnOverrun(false);
  Serial.begin();

  // Set up I2C
  Wire.setSpeed(I2C_CLK_SPEED);
  Wire.begin();

  // Default settings
  AirQualityWingSettings_t defaultSettings =
  { 20000, //Measurement Interval
    true,                 //Has HPMA115
    true,                 //Has CCS811
    true,                 //Has Si7021
    CCS811_ADDRESS,       //CCS811 address
    CCS811_INT_PIN,       //CCS811 intpin
    CCS811_RST_PIN,       //CCS811 rst pin
    CCS811_WAKE_PIN,      //CCS811 wake pin
    HPMA1150_EN_PIN       //HPMA int pin
  };

  // Setup & Begin Air Quality
  AirQual.setup(AirQualityWingEvent, defaultSettings);
  AirQual.begin();

  // Set up cloud variable
  Particle.function("set_interval", set_interval);

  // Startup message
  Serial.println("Air Quality Wing for Particle Mesh");

}

// loop() runs over and over again, as quickly as it can execute.
void loop() {

  uint32_t err_code = AirQual.process();
  if( err_code != success ) {

    switch(err_code) {
      case si7021_error:
         Particle.publish("err", "si7021" , PRIVATE, NO_ACK);
          Log.error("Error si7021");
      case ccs811_error:
         Particle.publish("err", "ccs811" , PRIVATE, NO_ACK);
          Log.error("Error ccs811");
      case hpma115_error:
         Particle.publish("err", "hpma115" , PRIVATE, NO_ACK);
          Log.error("Error hpma115");
      default:
        break;
    }

  }

}
```

## LICENSE
Copyright 2019 Jared Wolff (Circuit Dojo LLC)

Licensed under the GNU General Public License v3.0 license
