#ifndef __MODBUS_H__
#define __MODBUS_H__

#include "esp_log.h"
#include "mbcontroller.h"  // for mbcontroller defines and api

#define MB_TCP_PORT_NUMBER (CONFIG_FMB_TCP_PORT_DEFAULT)
#define MB_MDNS_PORT (502)

// Defines below are used to define register start address for each type of Modbus registers
#define HOLD_OFFSET(field) ((uint16_t)(offsetof(holding_reg_params_t, field) >> 1))
#define INPUT_OFFSET(field) ((uint16_t)(offsetof(input_reg_params_t, field) >> 1))
#define MB_REG_DISCRETE_INPUT_START (0x0000)
#define MB_REG_COILS_START (0x0000)
#define MB_REG_INPUT_START_AREA0 (INPUT_OFFSET(input_data0)) // register offset input area 0
#define MB_REG_INPUT_START_AREA1 (INPUT_OFFSET(input_data4)) // register offset input area 1
#define MB_REG_HOLDING_START_AREA0 (HOLD_OFFSET(holding_data0))
#define MB_REG_HOLDING_START_AREA1 (HOLD_OFFSET(holding_data4))

#define MB_PAR_INFO_GET_TOUT (10) // Timeout for get parameter info
#define MB_CHAN_DATA_MAX_VAL (10)
#define MB_CHAN_DATA_OFFSET (1.1f)

#define MB_READ_MASK (MB_EVENT_INPUT_REG_RD | MB_EVENT_HOLDING_REG_RD | MB_EVENT_DISCRETE_RD | MB_EVENT_COILS_RD)
#define MB_WRITE_MASK (MB_EVENT_HOLDING_REG_WR | MB_EVENT_COILS_WR)
#define MB_READ_WRITE_MASK (MB_READ_MASK | MB_WRITE_MASK)

#define MB_ID_BYTE0(id) ((uint8_t)(id))
#define MB_ID_BYTE1(id) ((uint8_t)(((uint16_t)(id) >> 8) & 0xFF))
#define MB_ID_BYTE2(id) ((uint8_t)(((uint32_t)(id) >> 16) & 0xFF))
#define MB_ID_BYTE3(id) ((uint8_t)(((uint32_t)(id) >> 24) & 0xFF))

#define MB_ID2STR(id) MB_ID_BYTE0(id), MB_ID_BYTE1(id), MB_ID_BYTE2(id), MB_ID_BYTE3(id)

#define MB_DEVICE_ID (uint32_t) CONFIG_FMB_CONTROLLER_SLAVE_ID

#define MB_SLAVE_ADDR (CONFIG_MB_SLAVE_ADDR)

#define MB_MDNS_INSTANCE(pref) pref "mb_slave_tcp"

void start_mdns_service(void);
void stop_mdns_service(void);

void setup_reg_data(void);

esp_err_t slave_init(mb_communication_info_t *comm_info);
esp_err_t slave_destroy(void);

#endif