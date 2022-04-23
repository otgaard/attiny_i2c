/*
 * i2c_master.h
 *
 * Created: 2022/04/23 06:48:03
 *  Author: otgaard
 */ 

#ifndef I2C_MASTER_H_
#define I2C_MASTER_H_

#define F_CPU 8000000UL

#include <avr/io.h>
#include <util/delay.h>

void setup_i2c();
void start_i2c();
void stop_i2c();
uint8_t receive_i2c(uint8_t ack);
uint8_t transmit_i2c(uint8_t data);

#endif /* I2C_MASTER_H_ */