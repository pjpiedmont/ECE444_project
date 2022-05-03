#include "sensors.h"
#include "i2c.h"
#include "INA209.h"
#include "esp_log.h"

#include "modbus_params.h" // for modbus parameters structures

void initINA()
{
	const char *TAG = "initINA";

	uint16_t cfgReg = 0;
	uint16_t cal = 0;

	INA209(I2C_MASTER_NUM, 0x40);

	writeCfgReg(0x3e67);
	// writeCfgReg(0x399f);
	cfgReg = readCfgReg();
	ESP_LOGI(TAG, "Config register = 0x%x", cfgReg);

	writeCal(0x6aaa);
	cal = readCal();
	ESP_LOGI(TAG, "Calibration register = 0x%x", cal);

	// v_queue = xQueueCreate(1, sizeof(float));
	// i_queue = xQueueCreate(1, sizeof(float));
	// p_queue = xQueueCreate(1, sizeof(float));

	// float v_dummy = 0.0f;
	// float i_dummy = 0.0f;
	// float p_dummy = 0.0f;

	// xQueueSend(v_queue, &v_dummy, 0);
	// xQueueSend(i_queue, &i_dummy, 0);
	// xQueueSend(p_queue, &p_dummy, 0);

	xTaskCreate(readINA, "readINA", 2048, NULL, 1, NULL);

	ESP_LOGI(TAG, "INA209 initialized successfully");
}

void readINA(void *pvParameters)
{

	int v_raw = 0;
	int i_raw = 0;
	int p_raw = 0;

	float v = 0.0f;
	float i = 0.0f;
	float p = 0.0f;

	while (1)
	{
		v_raw = busVol();
		v = (float)v_raw / 1000.0f;

		i_raw = current();
		i = (float)i_raw / 30.0f;

		p_raw = power();
		p = (float)p_raw * (2.0f/3.0f);
		
		input_reg_params.input_data0 = v;
		input_reg_params.input_data1 = i;
		input_reg_params.input_data2 = p;

		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}