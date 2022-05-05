/*
 * SPDX-FileCopyrightText: 2016-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// FreeModbus Master Example ESP32

// #include <string.h>
// #include <sys/queue.h>
// #include "esp_log.h"
// #include "esp_system.h"
// #include "esp_wifi.h"
// #include "esp_event.h"
// #include "esp_log.h"
// #include "nvs_flash.h"
// #include "esp_netif.h"
// #include "mdns.h"
#include "protocol_examples_common.h"

// #include "modbus_params.h" // for modbus parameters structures
// #include "mbcontroller.h"
// #include "sdkconfig.h"

#include "modbus.h"
#include "services.h"
#include "modbus_tasks.h"

extern char *slave_ip_address_table[];

void app_main(void)
{
	mb_tcp_addr_type_t ip_addr_type;
#if !CONFIG_EXAMPLE_CONNECT_IPV6
	ip_addr_type = MB_IPV4;
#else
	ip_addr_type = MB_IPV6;
#endif
	ESP_ERROR_CHECK(init_services(ip_addr_type));

	mb_communication_info_t comm_info = {0};
	comm_info.ip_port = MB_TCP_PORT;
	comm_info.ip_addr_type = ip_addr_type;
	comm_info.ip_mode = MB_MODE_TCP;
	comm_info.ip_addr = (void *)slave_ip_address_table;
	comm_info.ip_netif_ptr = (void *)get_example_netif();

	ESP_ERROR_CHECK(master_init(&comm_info));
	vTaskDelay(50);

	master_read(NULL);
	ESP_ERROR_CHECK(master_destroy());
	ESP_ERROR_CHECK(destroy_services());
}
