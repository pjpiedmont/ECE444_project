#include "modbus.h"

#include "esp_event.h"

#include "mdns.h"
#include "protocol_examples_common.h"

#include "modbus_params.h" // for modbus parameters structures

// convert mac from binary format to string
static inline char *gen_mac_str(const uint8_t *mac, char *pref, char *mac_str)
{
	sprintf(mac_str, "%s%02X%02X%02X%02X%02X%02X", pref, MAC2STR(mac));
	return mac_str;
}

static inline char *gen_id_str(char *service_name, char *slave_id_str)
{
	sprintf(slave_id_str, "%s%02X%02X%02X%02X", service_name, MB_ID2STR(MB_DEVICE_ID));
	return slave_id_str;
}

static inline char *gen_host_name_str(char *service_name, char *name)
{
	sprintf(name, "%s_%02X", service_name, MB_SLAVE_ADDR);
	return name;
}

void start_mdns_service(void)
{
	const char *TAG = "start_mdns_service";
	char temp_str[32] = {0};
	uint8_t sta_mac[6] = {0};
	ESP_ERROR_CHECK(esp_read_mac(sta_mac, ESP_MAC_WIFI_STA));
	char *hostname = gen_host_name_str(MB_MDNS_INSTANCE(""), temp_str);
	// initialize mDNS
	ESP_ERROR_CHECK(mdns_init());
	// set mDNS hostname (required if you want to advertise services)
	ESP_ERROR_CHECK(mdns_hostname_set(hostname));
	ESP_LOGI(TAG, "mdns hostname set to: [%s]", hostname);

	// set default mDNS instance name
	ESP_ERROR_CHECK(mdns_instance_name_set(MB_MDNS_INSTANCE("esp32_")));

	// structure with TXT records
	mdns_txt_item_t serviceTxtData[] = {
		{"board", "esp32"}};

	// initialize service
	ESP_ERROR_CHECK(mdns_service_add(hostname, "_modbus", "_tcp", MB_MDNS_PORT, serviceTxtData, 1));
	// add mac key string text item
	ESP_ERROR_CHECK(mdns_service_txt_item_set("_modbus", "_tcp", "mac", gen_mac_str(sta_mac, "\0", temp_str)));
	// add slave id key txt item
	ESP_ERROR_CHECK(mdns_service_txt_item_set("_modbus", "_tcp", "mb_id", gen_id_str("\0", temp_str)));
}

void stop_mdns_service(void)
{
	mdns_free();
}

// Set register values into known state
void setup_reg_data(void)
{
	// Define initial state of parameters
	discrete_reg_params.discrete_input0 = 0;
	discrete_reg_params.discrete_input1 = 0;
	discrete_reg_params.discrete_input2 = 0;
	discrete_reg_params.discrete_input3 = 0;
	discrete_reg_params.discrete_input4 = 0;
	discrete_reg_params.discrete_input5 = 0;
	discrete_reg_params.discrete_input6 = 0;
	discrete_reg_params.discrete_input7 = 0;

	holding_reg_params.holding_data0 = 0;
	holding_reg_params.holding_data1 = 0;
	holding_reg_params.holding_data2 = 0;
	holding_reg_params.holding_data3 = 0;
	holding_reg_params.holding_data4 = 0;
	holding_reg_params.holding_data5 = 0;
	holding_reg_params.holding_data6 = 0;
	holding_reg_params.holding_data7 = 0;

	coil_reg_params.coils_port0 = 0x00;
	coil_reg_params.coils_port1 = 0x00;

	input_reg_params.input_data0 = 0;
	input_reg_params.input_data1 = 0;
	input_reg_params.input_data2 = 0;
	input_reg_params.input_data3 = 0;
	input_reg_params.input_data4 = 0;
	input_reg_params.input_data5 = 0;
	input_reg_params.input_data6 = 0;
	input_reg_params.input_data7 = 0;
}

// Modbus slave initialization
esp_err_t slave_init(mb_communication_info_t *comm_info)
{
	const char *TAG = "slave_init";

	mb_register_area_descriptor_t reg_area; // Modbus register area descriptor structure

	void *slave_handler = NULL;

	// Initialization of Modbus controller
	esp_err_t err = mbc_slave_init_tcp(&slave_handler);
	MB_RETURN_ON_FALSE((err == ESP_OK && slave_handler != NULL), ESP_ERR_INVALID_STATE,
					   TAG,
					   "mb controller initialization fail.");

	comm_info->ip_addr = NULL; // Bind to any address
	comm_info->ip_netif_ptr = (void *)get_example_netif();

	// Setup communication parameters and start stack
	err = mbc_slave_setup((void *)comm_info);
	MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
					   TAG,
					   "mbc_slave_setup fail, returns(0x%x).",
					   (uint32_t)err);

	// The code below initializes Modbus register area descriptors
	// for Modbus Holding Registers, Input Registers, Coils and Discrete Inputs
	// Initialization should be done for each supported Modbus register area according to register map.
	// When external master trying to access the register in the area that is not initialized
	// by mbc_slave_set_descriptor() API call then Modbus stack
	// will send exception response for this register area.
	reg_area.type = MB_PARAM_HOLDING;							  // Set type of register area
	reg_area.start_offset = MB_REG_HOLDING_START_AREA0;			  // Offset of register area in Modbus protocol
	reg_area.address = (void *)&holding_reg_params.holding_data0; // Set pointer to storage instance
	reg_area.size = sizeof(float) << 2;							  // Set the size of register storage instance
	err = mbc_slave_set_descriptor(reg_area);
	MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
					   TAG,
					   "mbc_slave_set_descriptor fail, returns(0x%x).",
					   (uint32_t)err);

	reg_area.type = MB_PARAM_HOLDING;							  // Set type of register area
	reg_area.start_offset = MB_REG_HOLDING_START_AREA1;			  // Offset of register area in Modbus protocol
	reg_area.address = (void *)&holding_reg_params.holding_data4; // Set pointer to storage instance
	reg_area.size = sizeof(float) << 2;							  // Set the size of register storage instance
	err = mbc_slave_set_descriptor(reg_area);
	MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
					   TAG,
					   "mbc_slave_set_descriptor fail, returns(0x%x).",
					   (uint32_t)err);

	// Initialization of Input Registers area
	reg_area.type = MB_PARAM_INPUT;
	reg_area.start_offset = MB_REG_INPUT_START_AREA0;
	reg_area.address = (void *)&input_reg_params.input_data0;
	reg_area.size = sizeof(float) << 2;
	err = mbc_slave_set_descriptor(reg_area);
	MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
					   TAG,
					   "mbc_slave_set_descriptor fail, returns(0x%x).",
					   (uint32_t)err);
	reg_area.type = MB_PARAM_INPUT;
	reg_area.start_offset = MB_REG_INPUT_START_AREA1;
	reg_area.address = (void *)&input_reg_params.input_data4;
	reg_area.size = sizeof(float) << 2;
	err = mbc_slave_set_descriptor(reg_area);
	MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
					   TAG,
					   "mbc_slave_set_descriptor fail, returns(0x%x).",
					   (uint32_t)err);

	// Initialization of Coils register area
	reg_area.type = MB_PARAM_COIL;
	reg_area.start_offset = MB_REG_COILS_START;
	reg_area.address = (void *)&coil_reg_params;
	reg_area.size = sizeof(coil_reg_params);
	err = mbc_slave_set_descriptor(reg_area);
	MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
					   TAG,
					   "mbc_slave_set_descriptor fail, returns(0x%x).",
					   (uint32_t)err);

	// Initialization of Discrete Inputs register area
	reg_area.type = MB_PARAM_DISCRETE;
	reg_area.start_offset = MB_REG_DISCRETE_INPUT_START;
	reg_area.address = (void *)&discrete_reg_params;
	reg_area.size = sizeof(discrete_reg_params);
	err = mbc_slave_set_descriptor(reg_area);
	MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
					   TAG,
					   "mbc_slave_set_descriptor fail, returns(0x%x).",
					   (uint32_t)err);

	// Set values into known state
	setup_reg_data();

	// Starts of modbus controller and stack
	err = mbc_slave_start();
	MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
					   TAG,
					   "mbc_slave_start fail, returns(0x%x).",
					   (uint32_t)err);
	vTaskDelay(5);
	return err;
}

esp_err_t slave_destroy(void)
{
	const char *TAG = "slave_destroy";

	esp_err_t err = mbc_slave_destroy();
	MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
					   TAG,
					   "mbc_slave_destroy fail, returns(0x%x).",
					   (uint32_t)err);
	return err;
}