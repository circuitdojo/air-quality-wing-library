/*
 * Project Particle Squared
 * Description: Particle powered PM2.5 and air quality sensor
 * Author: Jared Wolff
 * Date: 4/27/2019
 * License: GNU GPLv3
 */

#include <math.h>
#include "sgp40.h"
#include "crc8_dallas.h"

SGP40::SGP40() {}

void SGP40::setReady()
{
  this->ready = true;
}

uint32_t SGP40::setup()
{

  // Init variables
  this->ready = false;
  this->data_available = false;
  this->has_env = false;
  this->log = new Logger("sgp40");

  // Start measurements
  uint8_t cmd[] = SGP40_MEAS_RAW_NO_HUM_OR_TEMP_CMD;
  Wire.beginTransmission(SGP40_ADDRESS);
  Wire.write(cmd, sizeof(cmd));         // sends register address
  uint8_t ret = Wire.endTransmission(); // stop transaction

  // Return an error if we have an error
  if (ret != 0)
  {
    this->log->error("sgp40 err: %d", ret);
    return SGP40_COMM_ERR;
  }

  // Set up algorithm
  VocAlgorithm_init(&this->voc_params);

  // Start sample timer to feed algorithm
  this->timer = new Timer(SGP40_READ_INTERVAL, [this](void) -> void
                          { return SGP40::setReady(); });

  this->timer->start();

  return SGP40_SUCCESS;
}

uint32_t SGP40::read(sgp40_data_t *p_data)
{

  if (this->data_available)
  {
    //Copy the data
    *p_data = this->data;

    return SGP40_SUCCESS;
  }
  else
  {
    return SGP40_NO_DAT_AVAIL;
  }
}

uint32_t SGP40::read_data_check_crc(uint16_t *data)
{

  // Read tvoc data and get CRC
  uint16_t temp = (Wire.read() << 8) + Wire.read();
  uint8_t crc_calc = crc8_dallas_little((uint8_t *)&temp, 2);
  uint8_t crc = Wire.read();

  // Return if CRC is incorrect
  if (crc != crc_calc)
  {
    this->log->error("crc fail %x %x", crc, crc_calc);
    return SGP40_DATA_ERR;
  }

  // Copy data over
  *data = temp;

  // Return on success!
  return SGP40_SUCCESS;
}

uint32_t SGP40::process()
{

  uint32_t err_code;

  if (this->ready)
  {

    // Reset this var
    this->ready = false;

    /* Prepare command */
    uint8_t cmd[] = SGP40_MEAS_RAW_NO_HUM_OR_TEMP_CMD;

    /* Copy the humidity and temperature settings if they exist */
    if (this->has_env)
    {
      memcpy(&cmd[2], &this->raw_humidity, sizeof(this->raw_humidity));
      memcpy(&cmd[5], &this->raw_temperature, sizeof(this->raw_temperature));
    }

    Wire.beginTransmission(SGP40_ADDRESS);
    Wire.write(cmd, sizeof(cmd));         // sends register address
    uint8_t ret = Wire.endTransmission(); // stop transaction

    // Return on error
    if (ret != 0)
    {
      this->log->error("error transfering bytes");
      return SGP40_COMM_ERR;
    }

    delay(30);

    uint8_t bytes_recieved, retries = 0;

    while (true)
    {

      // Start Rx 2 tvoc, 1 CRC
      bytes_recieved = Wire.requestFrom(WireTransmission(SGP40_ADDRESS).quantity(3));

      if (bytes_recieved)
        break;

      retries++;

      if (retries > 50)
      {
        this->log->error("exceeded retries.");
        return SGP40_COMM_ERR;
      }
    }

    // If no bytes recieved return
    if (bytes_recieved == 0 || bytes_recieved != 3)
    {
      this->log->error("byte count not matching. bytes %i", bytes_recieved);
      return SGP40_COMM_ERR;
    }

    // Get the TVOC data
    err_code = this->read_data_check_crc(&this->data.raw_tvoc);
    if (err_code != SGP40_SUCCESS)
    {
      this->log->error("crc failure");
      return SGP40_DATA_ERR;
    }

    // Feed SGP40 algorithm
    VocAlgorithm_process(&this->voc_params, this->data.raw_tvoc, &this->data.tvoc);

    // Print results
    this->log->info("raw: %d index: %d", this->data.raw_tvoc, (int)this->data.tvoc);

    // data is ready!
    this->data_available = true;
  }

  return SGP40_SUCCESS;
}

uint32_t SGP40::setEnv(uint8_t *raw_humidity, uint8_t *raw_temperature)
{

  memcpy(this->raw_humidity, raw_humidity, sizeof(this->raw_humidity));
  memcpy(this->raw_temperature, raw_temperature, sizeof(this->raw_temperature));
  this->has_env = true;

  return SGP40_SUCCESS;
}