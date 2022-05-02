#ifndef __INA209_H__
#define __INA209_H__

#include "driver/i2c.h"

void INA209(i2c_port_t num, uint8_t addr);

void pointReg(uint8_t reg_addr);
uint16_t readWord();
void writeWord(uint8_t reg_addr, uint16_t data);

uint16_t readCfgReg();
void writeCfgReg(uint16_t cfgReg);

uint16_t readCal();
void writeCal(uint16_t cal);

int busVol();
int power();
int current();

#endif