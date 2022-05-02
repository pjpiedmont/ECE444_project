/* i2c - Simple example

   Simple I2C example that shows how to initialize I2C
   as well as reading and writing from and to registers for a sensor connected over I2C.

   The sensor used in this example is a MPU9250 inertial measurement unit.

   For other examples please check:
   https://github.com/espressif/esp-idf/tree/master/examples

   See README.md file to get detailed usage of this example.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "INA209.h"
#include "comm.h"
#include <stdio.h>
#include "esp_log.h"

static const char *TAG = "i2c-simple-example";

void app_main(void)
{
	uint16_t cfgReg = 0;
	uint16_t cal = 0;

    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");

	INA209(I2C_MASTER_NUM, 0x40);

	writeCfgReg(0x3e67);
	cfgReg = readCfgReg();
	ESP_LOGI(TAG, "Config register = 0x%x", cfgReg);

	writeCal(0x6aaa);
	cal = readCal();
	ESP_LOGI(TAG, "Calibration register = 0x%x", cal);

	xTaskCreate(readINA, "readINA", 2048, NULL, 1, NULL);

    // ESP_ERROR_CHECK(i2c_driver_delete(I2C_MASTER_NUM));
    // ESP_LOGI(TAG, "I2C unitialized successfully");
}
