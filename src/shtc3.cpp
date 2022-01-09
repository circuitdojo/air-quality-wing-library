/*
 * Project Particle Squared
 * Description: Particle powered PM2.5 and air quality sensor
 * Author: Jared Wolff
 * Date: 2/26/2019
 * License: GNU GPLv3
 */

#include <math.h>

#include "shtc3.h"

SHTC3::SHTC3(void) {}

uint32_t SHTC3::setup()
{
  // Return error if we failed
  if (Wire.requestFrom(SHTC3_ADDRESS, 1) == 0)
  {
    return SHTC3_COMMS_FAIL_ERROR;
  }

  this->log = new Logger("shtc3");

  return SHTC3_SUCCESS;
}

uint32_t SHTC3::read(shtc3_data_t *p_data)
{

  // SHTC3 Temperature
  uint8_t temp_cmd[] = SHTC3_TEMP_HOLD_CMD;
  Wire.beginTransmission(SHTC3_ADDRESS);
  Wire.write(temp_cmd, sizeof(temp_cmd)); // sends bytes
  Wire.endTransmission();                 // stop transaction
  Wire.requestFrom(SHTC3_ADDRESS, 3);

  // Get the raw temperature from the device
  p_data->raw_temperature[0] = Wire.read() & 0xff;
  p_data->raw_temperature[1] = Wire.read() & 0xff;
  p_data->raw_temperature[2] = Wire.read() & 0xff;

  // Then calculate the temperature
  uint16_t temp = (p_data->raw_temperature[0] << 8) | p_data->raw_temperature[1];
  p_data->temperature = (temp * 175) / pow(2, 16) - 45;

  // SHTC3 Humidity
  uint8_t hum_cmd[] = SHTC3_HUMIDITY_HOLD_CMD;
  Wire.beginTransmission(SHTC3_ADDRESS);
  Wire.write(hum_cmd, sizeof(hum_cmd)); // sends bytes
  Wire.endTransmission();               // stop transaction
  Wire.requestFrom(SHTC3_ADDRESS, 3);

  // Get the raw humidity value from the evice
  p_data->raw_humidity[0] = Wire.read() & 0xff;
  p_data->raw_humidity[1] = Wire.read() & 0xff;
  p_data->raw_humidity[2] = Wire.read() & 0xff;

  // Then calculate the teperature
  uint16_t hum = (p_data->raw_humidity[0] << 8) | p_data->raw_humidity[1];
  p_data->humidity = hum * 100 / pow(2, 16);

  // Serial.printf("hum: %.2f%% temp: %.2f°C\n", p_data->humidity, p_data->temperature);

  return SHTC3_SUCCESS;
}