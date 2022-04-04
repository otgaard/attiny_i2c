/*
 * master.cpp
 *
 * Created: 2022/04/02 12:56:56
 * Author : otgaard
 */

#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>

#define FREQ_HZ 4000
#define DUR 1
#define STEP_DUR DUR
#define SCL (1 << 3)
#define SDA (1 << 4)

#define SLAVE_ADDR_READ 0b00011111
#define SLAVE_ADDR_WRITE 0b00011110

#define HIGH 1
#define LOW 0

/*
 * Note: We set the pins of SCL and SDA low and then change the data direction register to change the pin from
 * input to output.  When the pin is configured as an input, the line is pulled high, when the pin is configured
 * as an output, the line is pulled low via open-drain.
 * 4k7 resistors are used to pull both lines to Vcc.
 */

inline void set_i2c_pin(uint8_t pinMask, uint8_t value) {
    // value = true, pin set to input (floating), value = false, pin set to output (open-drain)
   value ? (DDRB &= ~pinMask) : DDRB |= pinMask;
}

void setup_i2c() {
    // Set the SCL and SDA pins low - NEVER set to high
    PORTB &= ~(SCL | SDA);
    set_i2c_pin(SCL, HIGH);
    set_i2c_pin(SDA, HIGH);
}

void delay_i2c() {
    _delay_ms(STEP_DUR);
}

void start_i2c() {
    set_i2c_pin(SDA, HIGH);
    delay_i2c();
    set_i2c_pin(SCL, HIGH);
    delay_i2c();
    set_i2c_pin(SDA, LOW);
    delay_i2c();
    set_i2c_pin(SCL, LOW);
    delay_i2c();
    // We are now ready to send the address
}

void stop_i2c() {
    set_i2c_pin(SDA, LOW);
    delay_i2c();
    set_i2c_pin(SCL, HIGH);
    delay_i2c();
    set_i2c_pin(SDA, HIGH);
    delay_i2c();
}

uint8_t receive(uint8_t ack) {
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

uint8_t transmit(uint8_t data) {
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


int main(void) {
	setup_i2c();
	uint8_t data = 0;
	while(1) {
		start_i2c();
		_delay_ms(DUR);
		transmit(data);
		data += 1;
		_delay_ms(DUR);
	}
}
