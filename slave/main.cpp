/*
 * slave.cpp
 *
 * Created: 2022/04/03 06:18:30
 * Author : otgaard
 */

#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>

#define FREQ_HZ 40
#define DUR (1000/(2*FREQ_HZ))
#define STEP_DUR (DUR/4)
#define SCL (1 << 2) // Same USI for later porting
#define SDA (1 << 0)

#define SLAVE_ADDR_READ 0b00011111
#define SLAVE_ADDR_WRITE 0b00011110

#define HIGH 1
#define LOW 0
#define TRUE 1
#define FALSE 0

// 74HC595 shift register
#define SER_PIN_MASK (1 << 1) // serial data
#define RCLK_PIN_MASK (1 << 3) // latch
#define RSCLK_PIN_MASK (1 << 4) // serial register clock

inline void set_i2c_pin(uint8_t pinMask, uint8_t value) {
	// value = true, pin set to input (floating), value = false, pin set to output (open-drain)
	value ? (DDRB &= ~pinMask) : (DDRB |= pinMask);
}

void setup_i2c() {
    // Set the SCL and SDA pins low - NEVER set to high
    PORTB &= ~(SCL | SDA);
    set_i2c_pin(SCL, HIGH);
    set_i2c_pin(SDA, HIGH);
}

uint8_t check_i2c_start(uint8_t count=255) {
    uint8_t i = 0;
    if(!(PINB & SDA) && (PINB & SCL)) { // SDA pulled low, SCL still high
		while((PINB & SCL) && i < count) {
			++i;
		}
		if(i == count) return FALSE;
		return !(PINB & SDA) && !(PINB & SCL);
	}
	return FALSE;
}

uint8_t receive(uint8_t ack) {
    uint8_t data = 0;
    for(int i = 0; i != 8; ++i) {
        data <<= 1;
        while(!(PINB & SCL));
        if(PINB & SDA) data |= 1;
		while(PINB & SCL);
    }

    set_i2c_pin(SDA, ack);
    while(PINB & SCL);
    set_i2c_pin(SDA, HIGH);
	return data;
}

uint8_t transmit(uint8_t data) {
    for(int i = 0; i != 8; ++i) {
        while(PINB & SCL);
        set_i2c_pin(SDA, data & 0x80);
        data <<= 1;
    }

    while(!(PINB & SCL));
    uint8_t ack = PINB & SDA;
    while(PINB & SCL);
    set_i2c_pin(SDA, HIGH);
    return ack;
}

void shift_byte(uint8_t value) {
    PORTB &= ~RCLK_PIN_MASK;
    for(int i = 0; i != 8; ++i) {
        PORTB &= ~RSCLK_PIN_MASK;
        if(value & 0x80) {
            PORTB |= SER_PIN_MASK;
        } else {
            PORTB &= ~SER_PIN_MASK;
        }
        PORTB |= RSCLK_PIN_MASK;
        value <<= 1;
    }
    PORTB |= RCLK_PIN_MASK;
}

int main(void) {
    setup_i2c();
    DDRB |= (SER_PIN_MASK | RCLK_PIN_MASK | RSCLK_PIN_MASK);
    uint8_t reg = 0;
    while (1) {
        reg++;
        if(check_i2c_start()) {
			while(PINB & SCL);
            uint8_t data = receive(1);
            shift_byte(data);
        }
    }
}
