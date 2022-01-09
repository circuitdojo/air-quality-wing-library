/*
 * Project Particle Squared
 * Description: Particle powered PM2.5 and air quality sensor
 * Author: Jared Wolff
 * Date: 2/26/2019
 * License: GNU GPLv3
 */

#ifndef SHTC3_H
#define SHTC3_H

#include "application.h"

#define SHTC3_ADDRESS 0x70

#define SHTC3_TEMP_HOLD_CMD \
  {                         \
    0x7C, 0xA2              \
  }
#define SHTC3_HUMIDITY_HOLD_CMD \
  {                             \
    0x5C, 0x24                  \
  }

#define SHTC3_SLEEP \
  {                 \
    0xB0, 0x98      \
  }

#define SHTC3_WAKE \
  {                \
    0x35, 0x17     \
  }

// Error code
#define SHTC3_SUCCESS 0
#define SHTC3_COMMS_FAIL_ERROR 1

typedef struct
{
  uint8_t raw_temperature[3];
  uint8_t raw_humidity[3];
  float temperature;
  float humidity;
} shtc3_data_t;

class SHTC3
{
public:
  SHTC3(void);
  uint32_t setup();
  uint32_t read(shtc3_data_t *p_data);

private:
  Logger *log;
};

#endif