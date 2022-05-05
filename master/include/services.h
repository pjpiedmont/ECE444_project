#ifndef __SERVICES_H__
#define __SERVICES_H__

#include "esp_err.h"
#include "mbcontroller.h"

#define MDNS_INSTANCE "power monitor modbus client and web server"

void initialize_mdns(void);
esp_err_t init_services(mb_tcp_addr_type_t ip_addr_type);
esp_err_t destroy_services(void);

#endif