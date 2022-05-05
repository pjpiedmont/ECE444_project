#include "modbus.h"

#include <stdio.h>
#include <string.h>
// #include <sys/queue.h>
#include "esp_log.h"
// #include "esp_system.h"
// #include "esp_wifi.h"
// #include "esp_event.h"
// #include "nvs_flash.h"
// #include "esp_netif.h"
// #include "protocol_examples_common.h"

#include "modbus_params.h" // for modbus parameters structures
// #include "sdkconfig.h"

// Example Data (Object) Dictionary for Modbus parameters:
// The CID field in the table must be unique.
// Modbus Slave Addr field defines slave address of the device with correspond parameter.
// Modbus Reg Type - Type of Modbus register area (Holding register, Input Register and such).
// Reg Start field defines the start Modbus register number and Reg Size defines the number of registers for the characteristic accordingly.
// The Instance Offset defines offset in the appropriate parameter structure that will be used as instance to save parameter value.
// Data Type, Data Size specify type of the characteristic and its data size.
// Parameter Options field specifies the options that can be used to process parameter value (limits or masks).
// Access Mode - can be used to implement custom options for processing of characteristic (Read/Write restrictions, factory mode values and etc).
const mb_parameter_descriptor_t device_parameters[] = {
	// {CID, Param Name, Units, Modbus Slave Addr, Modbus Reg Type, Reg Start, Reg Size, Instance Offset, Data Type, Data Size, Parameter Options, Access Mode}
	{CID_VOLTAGE, STR("Voltage"), STR("V"), MB_DEVICE_ADDR1, MB_PARAM_INPUT, 0, 2,
	 INPUT_OFFSET(input_data0), PARAM_TYPE_FLOAT, 4, OPTS(-32, 32, 0.004), PAR_PERMS_READ_WRITE_TRIGGER},
	{CID_CURRENT, STR("Current"), STR("mA"), MB_DEVICE_ADDR1, MB_PARAM_INPUT, 2, 2,
	 INPUT_OFFSET(input_data1), PARAM_TYPE_FLOAT, 4, OPTS(-3276.7, 3276.7, 0.1), PAR_PERMS_READ_WRITE_TRIGGER},
	{CID_POWER, STR("Power"), STR("mW"), MB_DEVICE_ADDR1, MB_PARAM_INPUT, 4, 2,
	 INPUT_OFFSET(input_data2), PARAM_TYPE_FLOAT, 4, OPTS(-104854, 104854, 0.33), PAR_PERMS_READ_WRITE_TRIGGER},
	{CID_POWER_THRESHOLD, STR("Power Threshold"), STR("mW"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0, 2,
	 HOLD_OFFSET(holding_data0), PARAM_TYPE_FLOAT, 4, OPTS(-104854, 104854, 0.33), PAR_PERMS_READ_WRITE_TRIGGER},
	{CID_DUTY_CYCLE, STR("Duty Cycle"), STR("\%"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 2, 2,
	 HOLD_OFFSET(holding_data1), PARAM_TYPE_FLOAT, 4, OPTS(0, 100, 0.1), PAR_PERMS_READ_WRITE_TRIGGER},
	{CID_MOTOR_STATE, STR("Motor State"), STR("on/off"), MB_DEVICE_ADDR1, MB_PARAM_COIL, 0, 8,
	 COIL_OFFSET(coils_port0), PARAM_TYPE_U16, 2, OPTS(BIT0, 0, 1), PAR_PERMS_READ_WRITE_TRIGGER},
	{CID_MOTOR_ALARM, STR("Motor Alarm"), STR("on/off"), MB_DEVICE_ADDR1, MB_PARAM_COIL, 8, 8,
	 COIL_OFFSET(coils_port1), PARAM_TYPE_U16, 2, OPTS(BIT0, 0, 1), PAR_PERMS_READ_WRITE_TRIGGER}
};

// Calculate number of parameters in the table
const uint16_t num_device_parameters = (sizeof(device_parameters) / sizeof(device_parameters[0]));

// This table represents slave IP addresses that correspond to the short address field of the slave in device_parameters structure
// Modbus TCP stack shall use these addresses to be able to connect and read parameters from slave
char *slave_ip_address_table[] = {
	NULL,
	NULL,
	NULL,
	NULL
};

const size_t ip_table_sz = (size_t)(sizeof(slave_ip_address_table) / sizeof(slave_ip_address_table[0]));

LIST_HEAD(slave_addr_, slave_addr_entry_s)
slave_addr_list = LIST_HEAD_INITIALIZER(slave_addr_list);

// convert MAC from binary format to string
/*static inline*/ char *gen_mac_str(const uint8_t *mac, char *pref, char *mac_str)
{
	sprintf(mac_str, "%s%02X%02X%02X%02X%02X%02X", pref, MAC2STR(mac));
	return mac_str;
}

/*static inline*/ char *gen_id_str(char *service_name, char *slave_id_str)
{
	sprintf(slave_id_str, "%s%02X%02X%02X%02X", service_name, MB_ID2STR(MB_DEVICE_ID));
	return slave_id_str;
}

// void master_start_mdns_service(void)
// {
// 	const char *TAG = "master_start_mdns_service";

// 	char temp_str[32] = {0};
// 	uint8_t sta_mac[6] = {0};
// 	ESP_ERROR_CHECK(esp_read_mac(sta_mac, ESP_MAC_WIFI_STA));
// 	char *hostname = gen_mac_str(sta_mac, MB_MDNS_INSTANCE("") "_", temp_str);
// 	// initialize mDNS
// 	ESP_ERROR_CHECK(mdns_init());
// 	// set mDNS hostname (required if you want to advertise services)
// 	ESP_ERROR_CHECK(mdns_hostname_set(hostname));
// 	ESP_LOGI(TAG, "mdns hostname set to: [%s]", hostname);

// 	// set default mDNS instance name
// 	ESP_ERROR_CHECK(mdns_instance_name_set(MB_MDNS_INSTANCE("esp32_")));

// 	// structure with TXT records
// 	mdns_txt_item_t serviceTxtData[] = {
// 		{"board", "esp32"}};

// 	// initialize service
// 	ESP_ERROR_CHECK(mdns_service_add(MB_MDNS_INSTANCE(""), "_modbus", "_tcp", MB_MDNS_PORT, serviceTxtData, 1));
// 	// add mac key string text item
// 	ESP_ERROR_CHECK(mdns_service_txt_item_set("_modbus", "_tcp", "mac", gen_mac_str(sta_mac, "\0", temp_str)));
// 	// add slave id key txt item
// 	ESP_ERROR_CHECK(mdns_service_txt_item_set("_modbus", "_tcp", "mb_id", gen_id_str("\0", temp_str)));
// }

char *master_get_slave_ip_str(mdns_ip_addr_t *address, mb_tcp_addr_type_t addr_type)
{
	mdns_ip_addr_t *a = address;
	char *slave_ip_str = NULL;

	while (a)
	{
		if ((a->addr.type == ESP_IPADDR_TYPE_V6) && (addr_type == MB_IPV6))
		{
			if (-1 == asprintf(&slave_ip_str, IPV6STR, IPV62STR(a->addr.u_addr.ip6)))
			{
				abort();
			}
		}
		else if ((a->addr.type == ESP_IPADDR_TYPE_V4) && (addr_type == MB_IPV4))
		{
			if (-1 == asprintf(&slave_ip_str, IPSTR, IP2STR(&(a->addr.u_addr.ip4))))
			{
				abort();
			}
		}
		if (slave_ip_str)
		{
			break;
		}
		a = a->next;
	}
	return slave_ip_str;
}

esp_err_t master_resolve_slave(uint8_t short_addr, mdns_result_t *result, char **resolved_ip,
							   mb_tcp_addr_type_t addr_type)
{
	const char *TAG = "master_resolve_slave";

	if (!short_addr || !result || !resolved_ip)
	{
		return ESP_ERR_INVALID_ARG;
	}
	mdns_result_t *r = result;
	int t;
	char *slave_ip = NULL;
	char slave_name[22] = {0};

	if (sprintf(slave_name, "mb_slave_tcp_%02X", short_addr) < 0)
	{
		ESP_LOGE(TAG, "Fail to create instance name for index: %d", short_addr);
		abort();
	}
	for (; r; r = r->next)
	{
		if ((r->ip_protocol == MDNS_IP_PROTOCOL_V4) && (addr_type == MB_IPV6))
		{
			continue;
		}
		else if ((r->ip_protocol == MDNS_IP_PROTOCOL_V6) && (addr_type == MB_IPV4))
		{
			continue;
		}
		// Check host name for Modbus short address and
		// append it into slave ip address table
		if ((strcmp(r->instance_name, slave_name) == 0) && (r->port == CONFIG_FMB_TCP_PORT_DEFAULT))
		{
			printf("  PTR : %s\n", r->instance_name);
			if (r->txt_count)
			{
				printf("  TXT : [%u] ", r->txt_count);
				for (t = 0; t < r->txt_count; t++)
				{
					printf("%s=%s; ", r->txt[t].key, r->txt[t].value ? r->txt[t].value : "NULL");
				}
				printf("\n");
			}
			slave_ip = master_get_slave_ip_str(r->addr, addr_type);
			if (slave_ip)
			{
				ESP_LOGI(TAG, "Resolved slave %s[%s]:%u", r->hostname, slave_ip, r->port);
				*resolved_ip = slave_ip;
				return ESP_OK;
			}
		}
	}
	*resolved_ip = NULL;
	ESP_LOGD(TAG, "Fail to resolve slave: %s", slave_name);
	return ESP_ERR_NOT_FOUND;
}

int master_create_slave_list(mdns_result_t *results, char **addr_table,
							 int addr_table_size, mb_tcp_addr_type_t addr_type)
{
	const char *TAG = "master_create_slave_list";

	if (!results)
	{
		return -1;
	}
	int i, slave_addr, cid_resolve_cnt = 0;
	int ip_index = 0;
	const mb_parameter_descriptor_t *pdescr = &device_parameters[0];
	char **ip_table = addr_table;
	char *slave_ip = NULL;
	slave_addr_entry_t *it;

	for (i = 0; (i < num_device_parameters && pdescr); i++, pdescr++)
	{
		slave_addr = pdescr->mb_slave_addr;

		it = NULL;
		// Is the slave address already registered?
		LIST_FOREACH(it, &slave_addr_list, entries)
		{
			if (slave_addr == it->slave_addr)
			{
				break;
			}
		}
		if (!it)
		{
			// Resolve new slave IP address using its short address
			esp_err_t err = master_resolve_slave(slave_addr, results, &slave_ip, addr_type);
			if (err != ESP_OK)
			{
				ESP_LOGE(TAG, "Index: %d, sl_addr: %d, failed to resolve!", i, slave_addr);
				// Set correspond index to NULL indicate host not resolved
				ip_table[ip_index] = NULL;
				continue;
			}
			// Register new slave address information
			slave_addr_entry_t *new_slave_entry = (slave_addr_entry_t *)heap_caps_malloc(sizeof(slave_addr_entry_t),
																						 MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
			MB_RETURN_ON_FALSE((new_slave_entry != NULL), ESP_ERR_NO_MEM,
							   TAG, "Can not allocate memory for slave entry.");
			new_slave_entry->index = i;
			new_slave_entry->ip_address = slave_ip;
			new_slave_entry->slave_addr = slave_addr;
			new_slave_entry->p_data = NULL;
			LIST_INSERT_HEAD(&slave_addr_list, new_slave_entry, entries);
			ip_table[ip_index] = slave_ip;
			ESP_LOGI(TAG, "Index: %d, sl_addr: %d, resolved to IP: [%s]",
					 i, slave_addr, slave_ip);
			cid_resolve_cnt++;
			if (ip_index < addr_table_size)
			{
				ip_index++;
			}
		}
		else
		{
			ip_table[ip_index] = it ? it->ip_address : ip_table[ip_index];
			ESP_LOGI(TAG, "Index: %d, sl_addr: %d, set to IP: [%s]",
					 i, slave_addr, ip_table[ip_index]);
			cid_resolve_cnt++;
		}
	}
	ESP_LOGI(TAG, "Resolved %d cids, with %d IP addresses", cid_resolve_cnt, ip_index);
	return cid_resolve_cnt;
}

int master_query_slave_service(const char *service_name, const char *proto,
							   mb_tcp_addr_type_t addr_type)
{
	const char *TAG = "master_query_slave_service";

	ESP_LOGI(TAG, "Query PTR: %s.%s.local", service_name, proto);

	mdns_result_t *results = NULL;
	int count = 0;

	esp_err_t err = mdns_query_ptr(service_name, proto, 3000, 20, &results);
	if (err)
	{
		ESP_LOGE(TAG, "Query Failed: %s", esp_err_to_name(err));
		return count;
	}
	if (!results)
	{
		ESP_LOGW(TAG, "No results found!");
		return count;
	}

	count = master_create_slave_list(results, slave_ip_address_table, ip_table_sz, addr_type);

	mdns_query_results_free(results);
	return count;
}

void master_destroy_slave_list(char **table, size_t ip_table_size)
{
	slave_addr_entry_t *it;
	LIST_FOREACH(it, &slave_addr_list, entries)
	{
		LIST_REMOVE(it, entries);
		free(it);
	}
	for (int i = 0; ((i < ip_table_size) && table[i] != NULL); i++)
	{
		if (table[i])
		{
			table[i] = NULL;
		}
	}
}

// The function to get pointer to parameter storage (instance) according to parameter description table
void *master_get_param_data(const mb_parameter_descriptor_t *param_descriptor)
{
	const char *TAG = "master_get_param_data";

	assert(param_descriptor != NULL);
	void *instance_ptr = NULL;
	if (param_descriptor->param_offset != 0)
	{
		switch (param_descriptor->mb_param_type)
		{
		case MB_PARAM_HOLDING:
			instance_ptr = ((void *)&holding_reg_params + param_descriptor->param_offset - 1);
			break;
		case MB_PARAM_INPUT:
			instance_ptr = ((void *)&input_reg_params + param_descriptor->param_offset - 1);
			break;
		case MB_PARAM_COIL:
			instance_ptr = ((void *)&coil_reg_params + param_descriptor->param_offset - 1);
			break;
		case MB_PARAM_DISCRETE:
			instance_ptr = ((void *)&discrete_reg_params + param_descriptor->param_offset - 1);
			break;
		default:
			instance_ptr = NULL;
			break;
		}
	}
	else
	{
		ESP_LOGE(TAG, "Wrong parameter offset for CID #%d", param_descriptor->cid);
		assert(instance_ptr != NULL);
	}
	return instance_ptr;
}

// Modbus master initialization
esp_err_t master_init(mb_communication_info_t *comm_info)
{
	const char *TAG = "master_init";

	void *master_handler = NULL;

	esp_err_t err = mbc_master_init_tcp(&master_handler);
	MB_RETURN_ON_FALSE((master_handler != NULL), ESP_ERR_INVALID_STATE,
					   TAG,
					   "mb controller initialization fail.");
	MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
					   TAG,
					   "mb controller initialization fail, returns(0x%x).",
					   (uint32_t)err);

	err = mbc_master_setup((void *)comm_info);
	MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
					   TAG,
					   "mb controller setup fail, returns(0x%x).",
					   (uint32_t)err);

	err = mbc_master_set_descriptor(&device_parameters[0], num_device_parameters);
	MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
					   TAG,
					   "mb controller set descriptor fail, returns(0x%x).",
					   (uint32_t)err);
	ESP_LOGI(TAG, "Modbus master stack initialized...");

	err = mbc_master_start();
	MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
					   TAG,
					   "mb controller start fail, returns(0x%x).",
					   (uint32_t)err);
	vTaskDelay(5);
	return err;
}

esp_err_t master_destroy(void)
{
	const char *TAG = "master_destroy";
	
	esp_err_t err = mbc_master_destroy();
	MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE,
					   TAG,
					   "mbc_master_destroy fail, returns(0x%x).",
					   (uint32_t)err);
	ESP_LOGI(TAG, "Modbus master stack destroy...");
	return err;
}