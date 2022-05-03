#include "services.h"
#include "modbus.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "protocol_examples_common.h"

esp_err_t init_services(void)
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
	start_mdns_service();
	// This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
	// Read "Establishing Wi-Fi or Ethernet Connection" section in
	// examples/protocols/README.md for more information about this function.
	result = example_connect();
	MB_RETURN_ON_FALSE((result == ESP_OK), ESP_ERR_INVALID_STATE,
					   TAG,
					   "example_connect fail, returns(0x%x).",
					   (uint32_t)result);
	result = esp_wifi_set_ps(WIFI_PS_NONE);
	MB_RETURN_ON_FALSE((result == ESP_OK), ESP_ERR_INVALID_STATE,
					   TAG,
					   "esp_wifi_set_ps fail, returns(0x%x).",
					   (uint32_t)result);
	return ESP_OK;
}

esp_err_t destroy_services(void)
{
	const char *TAG = "destroy_services";

	esp_err_t err = ESP_OK;

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
	stop_mdns_service();
	return err;
}