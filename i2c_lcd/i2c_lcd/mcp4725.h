/*
 * mcp4725.h
 *
 * Created: 2022/04/24 14:00:07
 *  Author: otgaard
 */ 


#ifndef MCP4725_H_
#define MCP4725_H_

#include <avr/io.h>

#define MCP4725_ADDR_WRITE 0b11000000

void sine_wave(uint16_t* value);

#endif /* MCP4725_H_ */