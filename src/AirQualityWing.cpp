#include "AirQualityWing.h"

// Constructor
AirQualityWing::AirQualityWing() {}

// This fires after the hpma should have finished...
void AirQualityWing::hpmaTimerEvent()
{
  // Set ready flag if we time out
  this->measurementComplete = true;
  this->hpmaError = true;
}

// Async publish event
void AirQualityWing::hpmaEvent()
{

  this->hpmaMeasurementComplete = true;

  Log.trace("pm25 %dμg/m3 pm10 %dμg/m3\n", data.hpma115.data.pm25, data.hpma115.data.pm10);
}

// Measurement timer handler
void AirQualityWing::measureTimerEvent()
{
  this->measurementStart = true;
}

AirQualityWingError_t AirQualityWing::setup(AirQualityWingHandler_t handler, AirQualityWingSettings_t settings)
{

  uint32_t err_code = success;

  // Set the handler
  this->handler_ = handler;

  // Set the settings
  this->settings_ = settings;

  // Create timers
  this->measurementTimer = new Timer(this->settings_.interval, [this](void) -> void
                                     { return measureTimerEvent(); });
  this->hpmaTimer = new Timer(
      HPMA_TIMEOUT_MS, [this](void) -> void
      { return hpmaTimerEvent(); },
      true); // One shot enabled.

  // Reset variables
  this->measurementStart = false;
  this->measurementComplete = false;
  this->hpmaMeasurementComplete = false;
  this->hpmaError = false;

  // SGP40 setup
  if (this->settings_.hasSGP40)
  {
    err_code = sgp40.setup();
    if (err_code != SGP40_SUCCESS)
    {
      Log.error("sgp40 setup err %d\n", (int)err_code);
    }
  }

  // Has the Si7021 init it
  if (this->settings_.hasSHTC3)
  {
    // Init Si7021
    uint32_t err_code = shtc3.setup();
    if (err_code != success)
    {
      Log.error("shtc3 setup err %d\n", (int)err_code);
    }
  }

  // Has the HPMA115 init it
  if (this->settings_.hasHPMA115)
  {
    // Setup
    hpma115_init_t hpma115_init = {
        [this](void) -> void
        { return hpmaEvent(); },
        this->settings_.hpma115IntPin};

    // Init HPM115 sensor
    err_code = hpma115.setup(&hpma115_init);
    if (err_code != HPMA115_SUCCESS)
    {
      Log.error("hpma115 enable err %d\n", (int)err_code);
    }
  }

  return success;
}

AirQualityWingError_t AirQualityWing::begin()
{

  // Start the timer
  this->measurementTimer->start();

  // Start measurement
  this->measurementStart = true;

  return success;
}

void AirQualityWing::end()
{
}

String AirQualityWing::toString()
{

  String out = "{";

  // If we have SGP40 data, concat
  if (this->data.hpma115.hasData)
  {
    out = String(out + String::format("\"pm25\":%d,\"pm10\":%d", this->data.hpma115.data.pm25, this->data.hpma115.data.pm10));
  }

  // If we have Si7021 data, concat
  if (this->data.shtc3.hasData)
  {

    // Add comma
    if (this->data.hpma115.hasData)
    {
      out = String(out + ",");
    }

    out = String(out + String::format("\"temperature\":%.2f,\"humidity\":%.2f", this->data.shtc3.data.temperature, this->data.shtc3.data.humidity));
  }

  // If we have sgp40 data, concat
  if (this->data.sgp40.hasData)
  {

    // Add comma
    if (this->data.hpma115.hasData || this->data.shtc3.hasData)
    {
      out = String(out + ",");
    }

    out = String(out + String::format("\"tvoc\":%d", this->data.sgp40.data.tvoc));
  }

  return String(out + "}");
}

AirQualityWingData_t AirQualityWing::getData()
{
  return this->data;
}

void AirQualityWing::attachHandler(AirQualityWingHandler_t handler)
{
  this->handler_ = handler;
}

void AirQualityWing::deattachHandler()
{
  this->handler_ = nullptr;
}

AirQualityWingError_t AirQualityWing::process()
{

  uint32_t err_code = success;

  // Set flag if measurement is done!
  if (this->hpmaMeasurementComplete == true)
  {
    // Reset
    this->hpmaMeasurementComplete = false;

    Log.trace("hpma complete");

    // Disable hpma
    this->hpma115.disable();

    // Copy data
    this->data.hpma115.data = this->hpma115.getData();
    this->data.hpma115.hasData = true;

    // Set available flag
    this->measurementComplete = true;
  }

  // If we're greater than or equal to the measurement delay
  // start taking measurements!
  if (this->measurementStart)
  {

    Log.trace("measurement start");

    // Set state variable to false
    this->measurementStart = false;

    // Reset has data variables
    this->data.shtc3.hasData = false;
    this->data.hpma115.hasData = false;
    this->data.sgp40.hasData = false;

    // Disable HPMA
    if (this->settings_.hasHPMA115)
      hpma115.disable();

    if (this->settings_.hasSHTC3)
    {
      // Read temp and humiity
      err_code = shtc3.read(&this->data.shtc3.data);

      if (err_code == SHTC3_SUCCESS)
      {
        // Set has data flag
        this->data.shtc3.hasData = true;

        // Set env data in the SGP40
        if (this->settings_.hasSGP40)
        {
          sgp40.setEnv(this->data.shtc3.data.raw_humidity, this->data.shtc3.data.raw_temperature);
        }
      }
      else
      {
        Log.error("Error temp - fatal err");
        return shtc3_error;
      }
    }

    // Process SGP40
    if (this->settings_.hasSGP40)
    {
      err_code = sgp40.read(&this->data.sgp40.data);

      if (err_code == SGP40_SUCCESS)
      {
        // Set has data flag
        this->data.sgp40.hasData = true;
      }
      else if (err_code == SGP40_NO_DAT_AVAIL)
      {
        Log.warn("Error tvoc - no data");
      }
      else
      {
        Log.error("Error tvoc - fatal");
        // return sgp40_error;
      }
    }

    // Process PM2.5 and PM10 results
    // This is slightly different from the other readings
    // due to the fact that it should be shut off when not taking a reading
    // (extends the life of the device)
    if (this->settings_.hasHPMA115)
    {
      this->hpma115.enable();
      this->hpmaTimer->start();
    }
    else
    {
      this->measurementComplete = true;
    }

    return success;
  }

  if (this->settings_.hasSGP40)
  {
    // Processes any avilable serial data
    err_code = sgp40.process();

    if (err_code != SGP40_SUCCESS)
    {
      Log.error("sp40 process error. Error: %i", (int)err_code);
    }
  }

  // Only run process command if this setup has HPMA115
  if (this->settings_.hasHPMA115)
    hpma115.process();

  // Send event if complete
  if (this->measurementComplete)
  {

    Log.trace("measurement complete");

    // Set flag to false
    this->measurementComplete = false;

    // Only handle if we have an hpma
    if (this->settings_.hasHPMA115)
    {
      // Stop timer
      this->hpmaTimer->stop();

      //If error show it here
      if (this->hpmaError)
      {
        Log.error("hpma timeout");

        // Disable on error
        this->hpma115.disable();

        // Reset error flag
        this->hpmaError = false;

        // Return error
        return hpma115_error;
      }
    }

    // Call handler
    if (this->handler_ != nullptr)
      this->handler_();
  }

  return success;
}

void AirQualityWing::setInterval(uint32_t interval)
{

  if (interval >= MIN_MEASUREMENT_DELAY_MS)
  {

    // Set the interval
    this->settings_.interval = interval;

    Log.trace("update reading period %d\n", (int)interval);

    // Change period if variable is updated
    this->measurementTimer->changePeriod(interval);
  }
}