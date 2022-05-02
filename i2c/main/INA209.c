#include "INA209.h"

i2c_port_t i2c_num;
uint8_t device_addr;

void INA209(i2c_port_t num, uint8_t addr)
{
	i2c_num = num;
	device_addr = addr;
}

void pointReg(uint8_t reg_addr)
{
	i2c_master_write_to_device(i2c_num, device_addr, &reg_addr, sizeof(reg_addr), 1000 / portTICK_RATE_MS);
}

uint16_t readWord()
{
	uint8_t data[2];
	uint16_t word;

	i2c_master_read_from_device(i2c_num, device_addr, data, sizeof(data), 1000 / portTICK_RATE_MS);

	word = data[0] << 8 | data[1];
	return word;
}

void writeWord(uint8_t reg_addr, uint16_t data)
{
	uint8_t write_data[3] = {reg_addr, data >> 8, data & 0xFF};
	i2c_master_write_to_device(i2c_num, device_addr, write_data, sizeof(write_data), 1000 / portTICK_RATE_MS);
}

uint16_t readCfgReg()
{
	pointReg(0x00);
	return readWord();
}

void writeCfgReg(uint16_t cfgReg)
{
	writeWord(0x00, cfgReg);
}

uint16_t readCal()
{
	pointReg(0x16);
	return readWord();
}

void writeCal(uint16_t cal)
{
	writeWord(0x16, cal);
}

int busVol()
{
	pointReg(0x04);
	return (int)(readWord() >> 1);
}

int power()
{
	pointReg(0x05);
	return (int)readWord();
}

int current()
{
	pointReg(0x06);
	return (int)readWord();
}