/*
 * i2c_master.cpp
 *
 * Created: 2022/04/23 06:36:56
 *  Author: otgaard
 */ 

#include "i2c_master.h"

#define SCL (1 << 2)
#define SDA (1 << 0)

#define HIGH 1
#define LOW 0

#define I2C_FREQ 400000
#define I2C_DUR (1000000UL / (2*I2C_FREQ))

inline void set_i2c_pin(uint8_t pinMask, uint8_t value) {
   value ? (DDRB &= ~pinMask) : DDRB |= pinMask;
}

void setup_i2c() {
    PORTB &= ~(SCL | SDA);
    set_i2c_pin(SCL, HIGH);
    set_i2c_pin(SDA, HIGH);
}

void delay_i2c() {
    _delay_us(I2C_DUR);
}

void start_i2c() {
    set_i2c_pin(SDA, HIGH);
    set_i2c_pin(SCL, HIGH);
    delay_i2c();
    set_i2c_pin(SDA, LOW);
    delay_i2c();
    set_i2c_pin(SCL, LOW);
    delay_i2c();
}

void stop_i2c() {
    set_i2c_pin(SDA, LOW);
    delay_i2c();
    set_i2c_pin(SCL, HIGH);
    delay_i2c();
    set_i2c_pin(SDA, HIGH);
    delay_i2c();
}

uint8_t receive_i2c(uint8_t ack) {
    uint8_t data = 0;

    set_i2c_pin(SDA, HIGH);

    for(int i = 0; i != 8; ++i) {
        data <<= 1;
        set_i2c_pin(SCL, HIGH);
        delay_i2c();
        if(PINB & SDA) data |= 1;
        set_i2c_pin(SCL, LOW);
        delay_i2c();
    }

    set_i2c_pin(SDA, ack);
    delay_i2c();
    set_i2c_pin(SCL, HIGH);
    delay_i2c();
    set_i2c_pin(SCL, LOW);
    delay_i2c();
    set_i2c_pin(SDA, HIGH);
    delay_i2c();
	return data;
}

uint8_t transmit_i2c(uint8_t data) {
    for(int i = 0; i != 8; ++i) {
        set_i2c_pin(SDA, data & 0x80);
        set_i2c_pin(SCL, HIGH);
        delay_i2c();
        data <<= 1;
        set_i2c_pin(SCL, LOW);
        delay_i2c();
    }

    set_i2c_pin(SDA, HIGH);
    delay_i2c();
    set_i2c_pin(SCL, HIGH);
    delay_i2c();
    uint8_t ack = PINB & SDA;
    set_i2c_pin(SCL, LOW);
	set_i2c_pin(SDA, LOW);
    return ack;
}
