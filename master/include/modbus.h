#ifndef __MODBUS_H__
#define __MODBUS_H__

#include <stdint.h>
#include "mdns.h"
#include "mbcontroller.h"

#define MB_TCP_PORT (CONFIG_FMB_TCP_PORT_DEFAULT) // TCP port used by example

// The number of parameters that intended to be used in the particular control process
#define MASTER_MAX_CIDS 7

// Number of reading of parameters from slave
#define MASTER_MAX_RETRY (30)

// Timeout to update cid over Modbus
#define UPDATE_CIDS_TIMEOUT_MS (1000)
#define UPDATE_CIDS_TIMEOUT_TICS (UPDATE_CIDS_TIMEOUT_MS / portTICK_RATE_MS)

// Timeout between polls
#define POLL_TIMEOUT_MS (1)
#define POLL_TIMEOUT_TICS (POLL_TIMEOUT_MS / portTICK_RATE_MS)
#define MB_MDNS_PORT (502)

// The macro to get offset for parameter in the appropriate structure
#define HOLD_OFFSET(field) ((uint16_t)(offsetof(holding_reg_params_t, field) + 1))
#define INPUT_OFFSET(field) ((uint16_t)(offsetof(input_reg_params_t, field) + 1))
#define COIL_OFFSET(field) ((uint16_t)(offsetof(coil_reg_params_t, field) + 1))
#define DISCR_OFFSET(field) ((uint16_t)(offsetof(discrete_reg_params_t, field) + 1))
#define STR(fieldname) ((const char *)(fieldname))

// Options can be used as bit masks or parameter limits
#define OPTS(min_val, max_val, step_val)                   \
	{                                                      \
		.opt1 = min_val, .opt2 = max_val, .opt3 = step_val \
	}

#define MB_ID_BYTE0(id) ((uint8_t)(id))
#define MB_ID_BYTE1(id) ((uint8_t)(((uint16_t)(id) >> 8) & 0xFF))
#define MB_ID_BYTE2(id) ((uint8_t)(((uint32_t)(id) >> 16) & 0xFF))
#define MB_ID_BYTE3(id) ((uint8_t)(((uint32_t)(id) >> 24) & 0xFF))

#define MB_ID2STR(id) MB_ID_BYTE0(id), MB_ID_BYTE1(id), MB_ID_BYTE2(id), MB_ID_BYTE3(id)

#if CONFIG_FMB_CONTROLLER_SLAVE_ID_SUPPORT
#define MB_DEVICE_ID (uint32_t) CONFIG_FMB_CONTROLLER_SLAVE_ID
#else
#define MB_DEVICE_ID (uint32_t)0x00112233
#endif

#define MB_MDNS_INSTANCE(pref) pref "mb_master_tcp"

// Enumeration of modbus device addresses accessed by master device
// Each address in the table is a index of TCP slave ip address in mb_communication_info_t::tcp_ip_addr table
enum
{
	MB_DEVICE_ADDR1 = 1
};

// Enumeration of all supported CIDs for device (used in parameter definition table)
enum
{
	CID_VOLTAGE = 0,
	CID_CURRENT,
	CID_POWER,
	CID_POWER_THRESHOLD,
	CID_DUTY_CYCLE,
	CID_MOTOR_STATE,
	CID_MOTOR_ALARM,
	CID_COUNT
};

typedef struct slave_addr_entry_s
{
	uint16_t index;
	char *ip_address;
	uint8_t slave_addr;
	void *p_data;
	LIST_ENTRY(slave_addr_entry_s)
	entries;
} slave_addr_entry_t;

/*static inline*/ char *gen_mac_str(const uint8_t *mac, char *pref, char *mac_str);
/*static inline*/ char *gen_id_str(char *service_name, char *slave_id_str);

void master_start_mdns_service(void);

char *master_get_slave_ip_str(mdns_ip_addr_t *address, mb_tcp_addr_type_t addr_type);
esp_err_t master_resolve_slave(uint8_t short_addr, mdns_result_t *result, char **resolved_ip,
							   mb_tcp_addr_type_t addr_type);
int master_create_slave_list(mdns_result_t *results, char **addr_table,
							 int addr_table_size, mb_tcp_addr_type_t addr_type);
void master_destroy_slave_list(char **table, size_t ip_table_size);

int master_query_slave_service(const char *service_name, const char *proto,
							   mb_tcp_addr_type_t addr_type);

void *master_get_param_data(const mb_parameter_descriptor_t *param_descriptor);

esp_err_t master_init(mb_communication_info_t *comm_info);
esp_err_t master_destroy(void);

#endif