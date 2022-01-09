#ifndef AIR_QUALITY_H
#define AIR_QUALITY_H

#include "Particle.h"

#if (PLATFORM_ID != PLATFORM_ARGON) && (PLATFORM_ID != PLATFORM_BORON)
#error The Air Quality Wing Library only supports Argon and Boron.
#endif

#include "shtc3.h"
#include "sgp40.h"
#include "hpma115.h"
#include "stdbool.h"

// Delay and timing related contsants
#define MEASUREMENT_DELAY_S 120
#define MEASUREMENT_DELAY_MS (MEASUREMENT_DELAY_S * 1000)
#define MIN_MEASUREMENT_DELAY_MS 10000
#define HPMA_TIMEOUT_MS 10000

typedef enum
{
  success = 0,
  shtc3_error,
  sgp40_error,
  hpma115_error,
} AirQualityWingError_t;

// Structure for holding data.
typedef struct
{
  struct
  {
    bool hasData;
    sgp40_data_t data;
  } sgp40;
  struct
  {
    bool hasData;
    shtc3_data_t data;
  } shtc3;
  struct
  {
    bool hasData;
    hpma115_data_t data;
  } hpma115;
} AirQualityWingData_t;

typedef struct
{
  uint32_t interval;
  bool hasHPMA115;
  bool hasSGP40;
  bool hasSHTC3;
  uint8_t hpma115IntPin;
} AirQualityWingSettings_t;

// Handler defintion
typedef std::function<void()> AirQualityWingHandler_t;

// Air quality class. Only create one of these!
class AirQualityWing
{
private:
  // Private data used to store latest values
  AirQualityWingHandler_t handler_;
  AirQualityWingSettings_t settings_;

  // Sensor objects
  SHTC3 shtc3;
  HPMA115 hpma115;
  SGP40 sgp40;

  // Variables
  Timer *measurementTimer;
  Timer *hpmaTimer;

  // Static measurement timer event function
  void hpmaEvent();
  void measureTimerEvent();
  void hpmaTimerEvent();
  void sgpTimerEvent();

  // Static var
  bool measurementStart;
  bool measurementComplete;
  bool hpmaMeasurementComplete;
  bool hpmaError;

  // Data
  AirQualityWingData_t data;

public:
  // Using defaults
  AirQualityWing();

  // Inits the devices
  AirQualityWingError_t setup(AirQualityWingHandler_t handler, AirQualityWingSettings_t settings);

  // Begins data collection
  AirQualityWingError_t begin();

  // Stops data collection, de-inits
  void end();

  // Prints out a string representation in JSON of the data
  String toString();

  // Returns a copy of the full data structure
  AirQualityWingData_t getData();

  // Attaches event handler. Event handler fires when a round of data has completed successfully
  void attachHandler(AirQualityWingHandler_t handler);

  // Deattaches event handler. The only way to fetch new data is using the `data()` method
  void deattachHandler();

  // Process method is required to process data correctly. Place in `loop()` function
  AirQualityWingError_t process();

  // Set measurement interval.
  // Accepts intervals from 20 seconds
  void setInterval(uint32_t interval);
};

#endif