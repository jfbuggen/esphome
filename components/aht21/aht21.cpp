// Implementation based on:
//  - Official manufacturer page: http://www.aosong.com/en/products-60.html
//    Official manufacturer sample program:  http://aosong.com/userfiles/files/software/AHT20-21%20DEMO%20V1_3(1).rar
//

#include "aht21.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace aht21 {

static const char *const TAG = "aht21";

// Delays in ms
static const uint32_t AHT21_STARTUP_DELAY = 500;
static const uint32_t AHT21_INIT_DELAY = 10;
static const uint32_t AHT21_SHORT_DELAY = 5;
static const uint32_t AHT21_MEDIUM_DELAY = 10;
static const uint32_t AHT21_LONG_DELAY = 75;        // Was 80 but state machine will wait an additional short delay so in total it stays 80
static const uint32_t AHT21_READ_INTERVAL = 300000;


// Initialisation is performed by writing specific values (2 bytes) to registers 0xA8 and 0xBE
// Note that AHT10 and AHT21 only have one register, but AHT21 apparently has two
static const uint8_t AHT21_INITIALIZE_CMD1[] = {0xA8, 0x00, 0x00 };
static const uint8_t AHT21_INITIALIZE_CMD2[] = {0xBE, 0x08, 0x00 };

// Additional initialisation is required if status bits 3-4 are set (0x18)
// In such case, three registers (0x1B, 0x1C and 0x1E) need to be set according to a specific process (see reset_reg_)
static const uint8_t AHT21_STATUS_INIT = 0x18;      // Bits 3-4 indicate that registers need to be reset
static const uint8_t AHT21_RESET_REG1 = 0x1B;
static const uint8_t AHT21_RESET_REG2 = 0x1C;
static const uint8_t AHT21_RESET_REG3 = 0x1E;
static const uint8_t AHT21_RESET_REG_MASK = 0xB0;


// Measure is triggered by writing two bytes to register 0xAC
static const uint8_t AHT21_MEASURE_CMD[] = {0xAC, 0x33, 0x00};

// While measure process is ongoing, the busy bit is set
static const uint8_t AHT21_STATUS_BUSY = 0x80;  // Bit 7 indicates that the device is busy

void AHT21Component::setup() {

  ESP_LOGV(TAG, "Setup()");
    
  if (this->power_pin_ == nullptr)
  {
    ESP_LOGE(TAG, "No power GPIO set!");
    this->mark_failed();
    return;
  }

  this->power_pin_->setup();
  this->state_ = AHT21state::idle;
  this->lastread_ = 0;
  this->lasttime_ = 0;
}

void AHT21Component::loop() {

    switch(this->state_)
    {
        case AHT21state::idle:
            // Check if it is time to make a new reading
            if ((millis() - this->lastread_) > AHT21_READ_INTERVAL)
            {
                this->state_ = AHT21state::start;
                this->lastread_ = millis();
            }
            break;
            
        case AHT21state::start:
            ESP_LOGV(TAG, "Starting");
            // Power on AHT21
            this->power_pin_->digital_write(true);
            this->lasttime_ = millis();
            this->state_ = AHT21state::init;
            break;
        
        case AHT21state::init:
            // Check if enough time was given for AHT21 to startup
            if ((millis() - this->lasttime_) >= AHT21_STARTUP_DELAY) {
                ESP_LOGV(TAG, "Initialising");
                // Perform initialisation
                if (this->init_()) {
                    if (this->should_reset_())
                    {
                        ESP_LOGV(TAG, "Resetting registers");
                        this->state_ = AHT21state::reset1;
                    }
                    else
                    {
                        this->state_ = AHT21state::ready;
                    }
                }
                else // initialisation not successful. Will retry later
                {
                    this->status_set_warning("Could not initialise!");
                    this->state_ = AHT21state::finish;
                }
            }
            break;

        case AHT21state::reset1:
            if (this->reset1_()) {
                this->state_ = AHT21state::reset2;
            }
            else
            {
                this->status_set_warning("Could not reset first register!");
                this->state_ = AHT21state::finish;
            }
            break;

        case AHT21state::reset2:
            if (this->reset2_()) {
                this->state_ = AHT21state::reset3;
            }
            else
            {
                this->status_set_warning("Could not reset second register!");
                this->state_ = AHT21state::finish;
            }
            break;

        case AHT21state::reset3:
            if (this->reset3_()) {
                this->state_ = AHT21state::ready;
            }
            else
            {
                this->status_set_warning("Could not reset third register!");
                this->state_ = AHT21state::finish;
            }
            break;
            
        case AHT21state::ready:
            ESP_LOGV(TAG, "Ready to start");
            this->lasttime_ = millis();
            this->state_ = AHT21state::start_measure;
            break;
        
        case AHT21state::start_measure:
            // Check if enough time was given for AHT21 to initialise
            if ((millis() - this->lasttime_) >= AHT21_INIT_DELAY) {
                // 
                ESP_LOGV(TAG, "Request");
                if (this->request_()) {
                    this->lasttime_ = millis();
                    this->state_ = AHT21state::wait_measure;
                }
                else
                {
                    this->status_set_warning("Could not request measure!");
                    this->state_ = AHT21state::finish;
                }
            }
            break;
            
        case AHT21state::wait_measure:
            // Give some time to AHT21 to perform measure
            if ((millis() - this->lasttime_) >= AHT21_LONG_DELAY) {
                this->count_ = 0;
                this->lasttime_ = millis();
                this->state_ = AHT21state::check_measure;
            }
            break;
            
        case AHT21state::check_measure:
            if ((millis() - this->lasttime_) >= AHT21_SHORT_DELAY) {
                if (this->busy_()) {
                    this->count_++;
                    this->lasttime_ = millis();
                    if (this->count_ >= 100)
                    {
                        this->status_set_warning("Cound not obtain measure!");
                        this->state_ = AHT21state::finish;
                    }
                }
                else
                {
                    this->state_ = AHT21state::read_measure;
                }
            }
            break;
                
        case AHT21state::read_measure:
            ESP_LOGV(TAG, "Retrieve");
            if (this->retrieve_())
            {
                this->status_clear_warning();

            }
            else
            {
                this->status_set_warning("Cound not retrieve measure!");
            }
            this->state_ = AHT21state::finish;
            break;
            
        case AHT21state::finish:
            ESP_LOGV(TAG, "Sleep");
            this->power_pin_->digital_write(false);
            this->state_ = AHT21state::idle;
            ESP_LOGD(TAG, "Measure took %d ms", (millis() - this->lastread_));
            break;
    }
    
}

// Checks AHT21 status against specific bit mask (returns true if all bits are set)
bool AHT21Component::check_status_(uint8_t status) {
    uint8_t data = 0;

    // Read one status byte
    if (this->read(&data, 1) != i2c::ERROR_OK) {
        ESP_LOGW(TAG, "Could not read AHT21 status!");
        return false;
    }
    
    return ((data & status) == status); 
}

// AHT21 initialisation after boot-up
bool AHT21Component::init_() {

    // Send initialisation sequence
    if (this->write(AHT21_INITIALIZE_CMD1, sizeof(AHT21_INITIALIZE_CMD1)) != i2c::ERROR_OK) {
        return false;
    };
    if (this->write(AHT21_INITIALIZE_CMD2, sizeof(AHT21_INITIALIZE_CMD2))!= i2c::ERROR_OK) {
        return false;
    };
    
    return true;
}

// Check if 2nd initialisation sequence is needed
bool AHT21Component::should_reset_()
{
    return (this->check_status_(AHT21_STATUS_INIT));
}

bool AHT21Component::reset1_()
{
    return (this->reset_reg_(AHT21_RESET_REG1));
}

bool AHT21Component::reset2_()
{
    return (this->reset_reg_(AHT21_RESET_REG2));
}

bool AHT21Component::reset3_()
{
    return (this->reset_reg_(AHT21_RESET_REG3));
}

// AHT21 register reset sequence
bool AHT21Component::reset_reg_(uint8_t reg) {
    uint8_t data[] = { reg, 0x00, 0x00 };

    ESP_LOGV(TAG, "Resetting register %2x", reg);

    // First step, write register + 2 null bytes
    if (this->write(data, sizeof(data)) != i2c::ERROR_OK) {
        ESP_LOGW(TAG, "Error resetting register! (step 1)");
        return false;
    }
  
    delay(AHT21_SHORT_DELAY);
  
    // Second step, get reply by reading three status bytes
    if (this->read(data, sizeof(data)) != i2c::ERROR_OK) {
        ESP_LOGW(TAG, "Error resetting register! (step 2)");
        return false;
    }
    
    delay(AHT21_MEDIUM_DELAY);
    
    // Third step, confirm reset, by writing back (mask | register) + last two status bytes
    data[0] = (AHT21_RESET_REG_MASK | reg); // Override first status byte
    if (this->write(data, sizeof(data)) != i2c::ERROR_OK) {
        ESP_LOGW(TAG, "Error resetting register! (step 3)");
        return false;
    }
    
    return true;
}

bool AHT21Component::request_()
{
    // First step, send "measure" command
    if (this->write(AHT21_MEASURE_CMD, sizeof(AHT21_MEASURE_CMD)) != i2c::ERROR_OK) {
        return false;
    }
    
    return true;
}

bool AHT21Component::busy_()
{
    return (this->check_status_(AHT21_STATUS_BUSY));
}

bool AHT21Component::retrieve_()
{
    uint8_t data[7] = { 0 };
    uint32_t humidity;
    uint32_t temperature;

    // Third step, read 6 bytes containing the sensor values
    if (this->read(data, sizeof(data)) != i2c::ERROR_OK) {
        ESP_LOGW(TAG, "Error reading measure!");
        return false;
    }
    
    // Check CRC
    if (this->crc8_(data,6) != data[6]) {       // Unfortunately esphome::crc8 does not give the expected result
        ESP_LOGW(TAG, "Bad CRC!");
        return false;
    }
    
    ESP_LOGV(TAG, "Dump %s", format_hex_pretty(data,7).c_str());
    
    // data[0] = status byte, not used here
    uint32_t raw_temperature = ((data[3] & 0x0F) << 16) | (data[4] << 8) | data[5];
    uint32_t raw_humidity = ((data[1] << 16) | (data[2] << 8) | data[3]) >> 4;
   
    if (this->temperature_sensor_ != nullptr) {
        float temperature = ((200.0f * (float) raw_temperature) / 1048576.0f) - 50.0f;
        this->temperature_sensor_->publish_state(temperature);
    }
    
    if (this->humidity_sensor_ != nullptr) {
        float humidity;
        if (raw_humidity == 0) {  // unrealistic value
          humidity = NAN;
        } else {
          humidity = (float) raw_humidity * 100.0f / 1048576.0f;
        }
        if (std::isnan(humidity)) {
          ESP_LOGW(TAG, "Invalid humidity! Sensor reported 0%% Hum");
        }
        this->humidity_sensor_->publish_state(humidity);
    }
    return true;
}

float AHT21Component::get_setup_priority() const { return setup_priority::DATA; }

void AHT21Component::dump_config() {
  ESP_LOGCONFIG(TAG, "AHT21:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with AHT21 failed!");
  }
  LOG_SENSOR("  ", "Temperature", this->temperature_sensor_);
  LOG_SENSOR("  ", "Humidity", this->humidity_sensor_);
}

uint8_t AHT21Component::crc8_(uint8_t *data, uint8_t len)
{
  uint8_t i;
  uint8_t byte;
  uint8_t crc=0xFF;
  
  for(byte=0; byte<len; byte++)
  {
    crc^=(data[byte]);
    for(i=8;i>0;--i)
    {
      if(crc&0x80) crc=(crc<<1)^0x31;
      else crc=(crc<<1);
    }
  }
  return crc;
}


}  // namespace aht21
}  // namespace esphome
