/*
 * SPDX-FileCopyrightText: 2016-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// FreeModbus Slave Example ESP32

// #include <stdio.h>
// #include "esp_err.h"
// #include "sdkconfig.h"
#include "esp_log.h"
// #include "esp_system.h"
// #include "esp_wifi.h"
// #include "esp_event.h"
// #include "nvs_flash.h"

// #include "mdns.h"
// #include "esp_netif.h"
// #include "protocol_examples_common.h"

// #include "mbcontroller.h"  // for mbcontroller defines and api
#include "modbus_params.h" // for modbus parameters structures

#include "i2c.h"
#include "sensors.h"
#include "INA209.h"
#include "modbus.h"
#include "services.h"

static const char *TAG = "rtu";

static portMUX_TYPE param_lock = portMUX_INITIALIZER_UNLOCKED;

static void slave_operation_func(void *arg)
{
	mb_param_info_t reg_info; // keeps the Modbus registers access information

	ESP_LOGI(TAG, "Modbus slave stack initialized.");
	ESP_LOGI(TAG, "Start modbus server...");
	// The cycle below will be terminated when parameter holding_data0
	// incremented each access cycle reaches the CHAN_DATA_MAX_VAL value.
	while (1)
	{
		// Check for read/write events of Modbus master for certain events
		mb_event_group_t event = mbc_slave_check_event(MB_READ_WRITE_MASK);
		const char *rw_str = (event & MB_READ_MASK) ? "READ" : "WRITE";
		// Filter events and process them accordingly
		if (event & (MB_EVENT_HOLDING_REG_WR | MB_EVENT_HOLDING_REG_RD))
		{
			// Get parameter information from parameter queue
			ESP_ERROR_CHECK(mbc_slave_get_param_info(&reg_info, MB_PAR_INFO_GET_TOUT));
			ESP_LOGI(TAG, "HOLDING %s (%u us), ADDR:%u, TYPE:%u, INST_ADDR:0x%.4x, SIZE:%u",
					 rw_str,
					 (uint32_t)reg_info.time_stamp,
					 (uint32_t)reg_info.mb_offset,
					 (uint32_t)reg_info.type,
					 (uint32_t)reg_info.address,
					 (uint32_t)reg_info.size);
			if (reg_info.address == (uint8_t *)&holding_reg_params.holding_data0)
			{
				portENTER_CRITICAL(&param_lock);
				// holding_reg_params.holding_data0 += MB_CHAN_DATA_OFFSET;
				if (holding_reg_params.holding_data0 >= (MB_CHAN_DATA_MAX_VAL - MB_CHAN_DATA_OFFSET))
				{
					coil_reg_params.coils_port1 = 0xFF;
				}
				portEXIT_CRITICAL(&param_lock);
			}
		}
		else if (event & MB_EVENT_INPUT_REG_RD)
		{
			ESP_ERROR_CHECK(mbc_slave_get_param_info(&reg_info, MB_PAR_INFO_GET_TOUT));
			ESP_LOGI(TAG, "INPUT READ (%u us), ADDR:%u, TYPE:%u, INST_ADDR:0x%.4x, SIZE:%u",
					 (uint32_t)reg_info.time_stamp,
					 (uint32_t)reg_info.mb_offset,
					 (uint32_t)reg_info.type,
					 (uint32_t)reg_info.address,
					 (uint32_t)reg_info.size);
		}
		else if (event & MB_EVENT_DISCRETE_RD)
		{
			ESP_ERROR_CHECK(mbc_slave_get_param_info(&reg_info, MB_PAR_INFO_GET_TOUT));
			ESP_LOGI(TAG, "DISCRETE READ (%u us): ADDR:%u, TYPE:%u, INST_ADDR:0x%.4x, SIZE:%u",
					 (uint32_t)reg_info.time_stamp,
					 (uint32_t)reg_info.mb_offset,
					 (uint32_t)reg_info.type,
					 (uint32_t)reg_info.address,
					 (uint32_t)reg_info.size);
		}
		else if (event & (MB_EVENT_COILS_RD | MB_EVENT_COILS_WR))
		{
			ESP_ERROR_CHECK(mbc_slave_get_param_info(&reg_info, MB_PAR_INFO_GET_TOUT));
			ESP_LOGI(TAG, "COILS %s (%u us), ADDR:%u, TYPE:%u, INST_ADDR:0x%.4x, SIZE:%u",
					 rw_str,
					 (uint32_t)reg_info.time_stamp,
					 (uint32_t)reg_info.mb_offset,
					 (uint32_t)reg_info.type,
					 (uint32_t)reg_info.address,
					 (uint32_t)reg_info.size);
			if (coil_reg_params.coils_port1 == 0xFF)
				break;
		}
	}
	// Destroy of Modbus controller on alarm
	ESP_LOGI(TAG, "Modbus controller destroyed.");
	vTaskDelay(100);
}

// An example application of Modbus slave. It is based on freemodbus stack.
// See deviceparams.h file for more information about assigned Modbus parameters.
// These parameters can be accessed from main application and also can be changed
// by external Modbus master host.
void app_main(void)
{
	initI2C();
	initINA();

	ESP_ERROR_CHECK(init_services());

	// Set UART log level
	esp_log_level_set(TAG, ESP_LOG_INFO);

	mb_communication_info_t comm_info = {0};

#if !CONFIG_EXAMPLE_CONNECT_IPV6
	comm_info.ip_addr_type = MB_IPV4;
#else
	comm_info.ip_addr_type = MB_IPV6;
#endif
	comm_info.ip_mode = MB_MODE_TCP;
	comm_info.ip_port = MB_TCP_PORT_NUMBER;
	ESP_ERROR_CHECK(slave_init(&comm_info));

	// The Modbus slave logic is located in this function (user handling of Modbus)
	slave_operation_func(NULL);

	ESP_ERROR_CHECK(slave_destroy());
	ESP_ERROR_CHECK(destroy_services());
}
