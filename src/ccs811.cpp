/*
 * Project Particle Squared
 * Description: Particle powered PM2.5 and air quality sensor
 * Author: Jared Wolff
 * Date: 2/26/2019
 * License: GNU GPLv3
 */

#include "ccs811.h"

CCS811::CCS811() {}

void CCS811::int_handler() {
  this->data_ready = true;
}

uint32_t CCS811::setup( ccs811_init_t * p_init ) {

  // Return an error if init is NULL
  if( p_init == NULL ) {
    return CCS811_NULL_ERROR;
  }

  // Configures the important stuff
  this->int_pin       = p_init->int_pin;
  this->address       = p_init->address;
  this->rst_pin       = p_init->rst_pin;
  this->wake_pin      = p_init->wake_pin;

  // Configure the pin interrupt
  pinMode(this->int_pin, INPUT);
  attachInterrupt(this->int_pin, &CCS811::int_handler, this, FALLING);

  // Wake it up
  pinMode(this->wake_pin, OUTPUT);
  digitalWrite(this->wake_pin, LOW); // Has a pullup

  // Toggle reset pin
  pinMode(this->rst_pin, OUTPUT);
  digitalWrite(this->rst_pin, LOW);
  delay(1);
  pinMode(this->rst_pin, INPUT);
  delay(30);

  // Run the app
  Wire.beginTransmission(this->address);
  Wire.write(CCS811_START_APP); // sends register address
  Wire.endTransmission();  // stop transaction

  delay(30);

  Wire.beginTransmission(this->address);
  Wire.write(CCS811_STATUS_REG);
  Wire.endTransmission();  // stop transaction
  Wire.requestFrom(this->address,(uint8_t)1);

  uint8_t status = Wire.read();

  // Checks if the app is running
  if( status & CCS811_FW_MODE_RUN ) {
    return CCS811_SUCCESS;
  } else {
    return CCS811_RUN_ERROR;
  }

}

uint32_t CCS811::set_env(float temp, float hum) {

  // Shift left b/c this register is shifted left by 1
  uint8_t hum_conv[2];
  hum_conv[0] = (uint8_t)hum << 1;
  hum_conv[1] = 0; // Not bothering with sending fraction

  // Serial.printf("temp %.2f", temp);
  // Serial.printf("hum %.2f", hum);
  // Serial.printf("frac %.2f", frac_part);

  // Generate the calculated values
  uint8_t temp_conv[2];
  temp_conv[0] = (((uint8_t)temp + 25) << 1);
	temp_conv[1] = 0; // Not bothering with sending fraction

  // Data to send
  uint8_t data[4];

  // Copy bytes to output
  memcpy(data,&hum_conv,sizeof(hum_conv));
  memcpy(data+2,&temp_conv,sizeof(temp_conv));

  // Write this
  Wire.beginTransmission(this->address);
  Wire.write(CCS811_ENV_REG);
  Wire.write(data,sizeof(data));
  Wire.endTransmission();  // stop transaction

  return CCS811_SUCCESS;

}

uint32_t CCS811::enable(void) {

  uint32_t err_code;

  // Set mode to 10 sec mode & enable int
  Wire.beginTransmission(this->address);
  Wire.write(CCS811_MEAS_MODE_REG); // sends register address
  Wire.write(CCS811_CONSTANT_MODE | CCS811_INT_EN);     // enables consant mode with interrupt
  err_code = Wire.endTransmission();           // stop transaction
  if( err_code != 0 ){
    return err_code;
  }

  // Clear any interrupts
  Wire.beginTransmission(this->address);
  Wire.write(CCS811_RESULT_REG); // sends register address
  err_code = Wire.endTransmission(false);  // stop transaction
  if( err_code != 0 ){
    return err_code;
  }

  // Flush bytes
  Wire.requestFrom(this->address, (uint8_t)4); // Read the bytes
  while(Wire.available()) {
    Wire.read();
  }

  return CCS811_SUCCESS;

}

uint32_t CCS811::save_baseline() {

  Wire.beginTransmission(this->address);
  Wire.write(CCS811_BASELINE_REG); // sends register address
  Wire.endTransmission();  // stop transaction
  Wire.requestFrom(this->address, (uint8_t)2); // request the bytes

  uint8_t baseline[2];
  baseline[0] = Wire.read();
  baseline[1] = Wire.read();

  // Write to the address
  EEPROM.put(CCS811_BASELINE_ADDR, baseline);

  return CCS811_SUCCESS;
}

uint32_t CCS811::restore_baseline() {

  uint32_t err_code;
  uint8_t baseline[2];

  // Get the baseline to the address
  EEPROM.get(CCS811_BASELINE_ADDR, baseline);

  // If it's uninitialized, return invalid data
  if ( baseline[0] == 0xff && baseline[1] == 0xff) {
    return CCS811_DAT_INVALID;
  }

  // Write to the chip
  Wire.beginTransmission(this->address);
  Wire.write(CCS811_BASELINE_REG); // sends register address
  Wire.write(baseline[0]);
  Wire.write(baseline[1]);
  err_code = Wire.endTransmission();           // stop transaction
  if( err_code != 0 ){
    return err_code;
  }

  return CCS811_SUCCESS;
}

uint32_t CCS811::read(ccs811_data_t * p_data) {

  // If the data is ready, read
  if( this->data_ready ) {

      // Disable flag
      this->data_ready = false;

      Wire.beginTransmission(this->address);
      Wire.write(CCS811_RESULT_REG); // sends register address
      Wire.endTransmission();  // stop transaction
      Wire.requestFrom(this->address, (uint8_t)4); // request the bytes

      // Convert data to something useful
      p_data->c02 = Wire.read();
      p_data->c02 = (p_data->c02<<8) + Wire.read();

      p_data->tvoc = Wire.read();
      p_data->tvoc = (p_data->tvoc<<8) + Wire.read();

      // Serial.printf("c02: %dppm tvoc: %dppb\n",p_data->c02,p_data->tvoc);

      // If this value is < 400 not ready yet
      if ( p_data->c02 < CCS811_MIN_C02_LEVEL ) {
        return CCS811_DAT_INVALID;
      }

      return CCS811_SUCCESS;
  } else {
      return CCS811_NO_DAT_AVAIL;
  }

}

uint32_t CCS811::get_app_version(ccs811_app_ver_t * p_app_ver) {

  uint32_t err_code;

  Wire.beginTransmission(this->address);
  Wire.write(CCS811_APP_VER_REG); // sends register address
  err_code = Wire.endTransmission();  // stop transaction
  if( err_code != 0) {
    return CCS811_COMM_ERR;
  }

  uint8_t num_bytes = Wire.requestFrom(this->address, (uint8_t)2); // request the bytes

  // If  not enough bytes were read, error!
  if( num_bytes != 2) {
    return CCS811_NO_DAT_AVAIL;
  }

  // Convert data to something useful
  uint8_t majorminor = Wire.read();

  // MSB is split into two. First nibble major, second is the  minor
  p_app_ver->major = (majorminor >> 4) & 0x0f;
  p_app_ver->minor = majorminor & 0x0f;
  p_app_ver->trivial = Wire.read();

  // Serial.printf("%x %x",majorminor,p_app_ver->trivial );

  return CCS811_SUCCESS;


}


uint32_t CCS811::update_app(const ccs811_app_update_t * p_app_update) {

  if( p_app_update == NULL ) {
    return CCS811_NULL_ERROR;
  }

  ccs811_app_ver_t current_ver;

  // Check the version
  uint32_t err_code = this->get_app_version(&current_ver);

  // Return if not success
  if( err_code != 0 ) {
    return err_code;
  }

  bool start_update = false;

  // If any of the version information is not equal, update
  if( p_app_update->ver.major > current_ver.major ||
      p_app_update->ver.minor > current_ver.minor ) {
    // start
    start_update = true;
  }

  // Return if there's no update
  if( !start_update ) {
    return CCS811_NO_UPDATE_NEEDED;
  }

  Serial.println("start update");

  // Otherwise there is an update
  // Toggle reset pin
  pinMode(this->rst_pin, OUTPUT);
  digitalWrite(this->rst_pin, LOW);
  delay(1);
  pinMode(this->rst_pin, INPUT);
  delay(30);

  // Erase codee
  uint8_t cmd[] = CCS811_ERASE_CODE;

  Serial.println("erase");

  // Begin the process
  Wire.beginTransmission(this->address);
  Wire.write(CCS811_ERASE_REG); // sends register address
  Wire.write(cmd, sizeof(cmd));
  err_code = Wire.endTransmission();  // stop transaction
  if( err_code != 0) {
    return CCS811_COMM_ERR;
  }

  delay(500);

  // Set up for ops
  uint32_t length = p_app_update->size;
  uint8_t *data = p_app_update->data;

  // Payload to be sent over I2C
  uint8_t payload[9];

  // Set first byte to the command
  payload[0] = CCS811_WRITE_APP_REG;

  // Calculate the iterations to run
  uint32_t iterations = length/8;

  Serial.printf("iterations %d", iterations);

  for( uint32_t i = 0; i < iterations; i++ ) {

    Serial.printf("%d\n",i);

    // Copy 8 bytes
    memcpy(&payload[1],data,8);

    // Write said 8 bytes
    Wire.beginTransmission(this->address);
    Wire.write(payload, sizeof(payload)); // sends register address
    err_code = Wire.endTransmission();  // stop transaction
    if( err_code != 0) {
      return CCS811_COMM_ERR;
    }

    // Delay
    delay(50);

    // Increment the pointer
    data+=8;

  }

  // Verify
  Wire.beginTransmission(this->address);
  Wire.write(CCS811_VERIFY_REG); // sends register address
  err_code = Wire.endTransmission();  // stop transaction
  if( err_code != 0) {
    return CCS811_COMM_ERR;
  }

  delay(500);

  // Check the status
  Wire.beginTransmission(this->address);
  Wire.write(CCS811_STATUS_REG);
  err_code = Wire.endTransmission();  // stop transaction
  if( err_code != 0) {
    return CCS811_COMM_ERR;
  }

  Wire.requestFrom(this->address,(uint8_t)1);

  uint8_t status = Wire.read();

  if ((status & 0x30) == 0x30){
    // program code valid
    Serial.println("Code is valid!");
    return CCS811_SUCCESS;
  }
  else{
    // program code invalid
    Serial.println("Code is valid!");
    return CCS811_UPDATE_VERIFY_FAIL;
  }



}
