#include "services.h"
#include "modbus.h"

// #include <string.h>
// #include <sys/queue.h>
// #include "esp_system.h"
#include "esp_wifi.h"
// #include "esp_event.h"
// #include "esp_log.h"
#include "nvs_flash.h"
// #include "esp_netif.h"
// #include "mdns.h"
#include "protocol_examples_common.h"

// #include "modbus_params.h" // for modbus parameters structures
// #include "mbcontroller.h"
// #include "sdkconfig.h"

#include "lwip/apps/netbiosns.h"

#include "modbus.h"
#include "rest_server.h"
#include "filesystem.h"

extern mb_parameter_descriptor_t device_parameters[];
extern uint16_t num_device_parameters;
extern char *slave_ip_address_table[];
extern size_t ip_table_sz;

void initialize_mdns(void)
{
	const char *TAG = "initialize_mdns";

	char temp_str[32] = {0};
	uint8_t sta_mac[6] = {0};
	ESP_ERROR_CHECK(esp_read_mac(sta_mac, ESP_MAC_WIFI_STA));
	char *hostname = CONFIG_EXAMPLE_MDNS_HOST_NAME; // gen_mac_str(sta_mac, MB_MDNS_INSTANCE("") "_", temp_str);
	// initialize mDNS
	ESP_ERROR_CHECK(mdns_init());
	// set mDNS hostname (required if you want to advertise services)
	ESP_ERROR_CHECK(mdns_hostname_set(hostname));
	ESP_LOGI(TAG, "mdns hostname set to: [%s]", hostname);

	// set default mDNS instance name
	ESP_ERROR_CHECK(mdns_instance_name_set(MDNS_INSTANCE));

	// structure with TXT records
	mdns_txt_item_t modbusTxtData[] = {
		{"board", "esp32"}};

	// initialize service
	ESP_ERROR_CHECK(mdns_service_add(MB_MDNS_INSTANCE(""), "_modbus", "_tcp", MB_MDNS_PORT, modbusTxtData, 1));
	// add mac key string text item
	ESP_ERROR_CHECK(mdns_service_txt_item_set("_modbus", "_tcp", "mac", gen_mac_str(sta_mac, "\0", temp_str)));
	// add slave id key txt item
	ESP_ERROR_CHECK(mdns_service_txt_item_set("_modbus", "_tcp", "mb_id", gen_id_str("\0", temp_str)));

	mdns_txt_item_t httpTxtData[] = {
        {"board", "esp32"},
        {"path", "/"}
    };

    ESP_ERROR_CHECK(mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80, httpTxtData,
                                     sizeof(httpTxtData) / sizeof(httpTxtData[0])));
}

// void app_main(void)
// {
//     ESP_ERROR_CHECK(nvs_flash_init());
//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());
//     initialise_mdns();
//     netbiosns_init();
//     netbiosns_set_name(CONFIG_EXAMPLE_MDNS_HOST_NAME);

//     ESP_ERROR_CHECK(example_connect());
//     ESP_ERROR_CHECK(init_fs());
//     ESP_ERROR_CHECK(start_rest_server(CONFIG_EXAMPLE_WEB_MOUNT_POINT));
// }

esp_err_t init_services(mb_tcp_addr_type_t ip_addr_type)
{
	const char *TAG = "init_services";

	esp_err_t result = nvs_flash_init();
	if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		result = nvs_flash_init();
	}
	MB_RETURN_ON_FALSE((result == ESP_OK), ESP_ERR_INVALID_STATE,
					   TAG,
					   "nvs_flash_init fail, returns(0x%x).",
					   (uint32_t)result);
	result = esp_netif_init();
	MB_RETURN_ON_FALSE((result == ESP_OK), ESP_ERR_INVALID_STATE,
					   TAG,
					   "esp_netif_init fail, returns(0x%x).",
					   (uint32_t)result);
	result = esp_event_loop_create_default();
	MB_RETURN_ON_FALSE((result == ESP_OK), ESP_ERR_INVALID_STATE,
					   TAG,
					   "esp_event_loop_create_default fail, returns(0x%x).",
					   (uint32_t)result);
	// Start mdns service and register device
	initialize_mdns();
	netbiosns_init();
	// This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
	// Read "Establishing Wi-Fi or Ethernet Connection" section in
	// examples/protocols/README.md for more information about this function.
	result = example_connect();
	MB_RETURN_ON_FALSE((result == ESP_OK), ESP_ERR_INVALID_STATE,
					   TAG,
					   "example_connect fail, returns(0x%x).",
					   (uint32_t)result);
#if CONFIG_EXAMPLE_CONNECT_WIFI
	result = esp_wifi_set_ps(WIFI_PS_NONE);
	MB_RETURN_ON_FALSE((result == ESP_OK), ESP_ERR_INVALID_STATE,
					   TAG,
					   "esp_wifi_set_ps fail, returns(0x%x).",
					   (uint32_t)result);
#endif

	ESP_ERROR_CHECK(init_fs());
    ESP_ERROR_CHECK(start_rest_server(CONFIG_EXAMPLE_WEB_MOUNT_POINT));

	int res = 0;
	for (int retry = 0; (res < num_device_parameters) && (retry < 10); retry++)
	{
		res = master_query_slave_service("_modbus", "_tcp", ip_addr_type);
	}
	if (res < num_device_parameters)
	{
		ESP_LOGE(TAG, "Could not resolve one or more slave IP addresses, resolved: %d out of %d.", res, num_device_parameters);
		ESP_LOGE(TAG, "Make sure you configured all slaves according to device parameter table and they alive in the network.");
		return ESP_ERR_NOT_FOUND;
	}
	mdns_free();

	return ESP_OK;
}

esp_err_t destroy_services(void)
{
	const char *TAG = "destroy_services";

	esp_err_t err = ESP_OK;
	master_destroy_slave_list(slave_ip_address_table, ip_table_sz);

	err = example_disconnect();
	MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
					   TAG,
					   "example_disconnect fail, returns(0x%x).",
					   (uint32_t)err);
	err = esp_event_loop_delete_default();
	MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
					   TAG,
					   "esp_event_loop_delete_default fail, returns(0x%x).",
					   (uint32_t)err);
	err = esp_netif_deinit();
	MB_RETURN_ON_FALSE((err == ESP_OK || err == ESP_ERR_NOT_SUPPORTED), ESP_ERR_INVALID_STATE,
					   TAG,
					   "esp_netif_deinit fail, returns(0x%x).",
					   (uint32_t)err);
	err = nvs_flash_deinit();
	MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
					   TAG,
					   "nvs_flash_deinit fail, returns(0x%x).",
					   (uint32_t)err);
	return err;
}