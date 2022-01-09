#ifndef SGP40_H
#define SGP40_H

#include "stdint.h"
#include "application.h"
#include "sensirion_voc_algorithm.h"

#define SGP40_ADDRESS 0x59

#define SGP40_MEAS_RAW_NO_HUM_OR_TEMP_CMD           \
  {                                                 \
    0x26, 0x0F, 0x80, 0x00, 0xA2, 0x66, 0x66, 0x93, \
  }
#define SGP40_HEATER_OFF_CMD \
  {                          \
    0x36, 0x15               \
  }
#define SGP40_SOFT_RST_CMD \
  {                        \
    0x00, 0x06             \
  }

#define SGP40_READ_INTERVAL 1000

// Error codes
enum
{
  SGP40_SUCCESS,
  SGP40_NULL_ERROR,
  SGP40_NO_DAT_AVAIL,
  SGP40_RUN_ERROR,
  SGP40_COMM_ERR,
  SGP40_DATA_ERR,
};

typedef struct
{
  uint16_t raw_tvoc;
  int32_t tvoc;
} sgp40_data_t;

class SGP40
{
public:
  SGP40(void);
  uint32_t setup();
  uint32_t enable(void);
  uint32_t setEnv(uint8_t *raw_humidity, uint8_t *raw_temperature);
  uint32_t read(sgp40_data_t *p_data);
  uint32_t process();

private:
  void setReady(void);
  uint32_t read_data_check_crc(uint16_t *data);
  Timer *timer;
  VocAlgorithmParams voc_params;

protected:
  uint8_t raw_temperature[3], raw_humidity[3];
  uint16_t has_env;
  sgp40_data_t data;
  bool ready, data_available;
  Logger *log;
};

#endif // SGP40_H