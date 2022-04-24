/*
 * i2c_lcd.cpp
 *
 * Created: 2022/04/16 09:14:22
 * Author : otgaard
 */ 

#include "i2c_master.h"
#include "mcp4725.h"
#include <stdlib.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#define SRCLK_PIN_MASK (1 << PB1)
#define SER_PIN_MASK (1 << PB3)
#define RCLK_PIN_MASK (1 << PB4)

void shift_byte(uint8_t byte) {
	for(uint8_t i = 0; i != 8; ++i) {
		byte & 0x80 ? PORTB |= SER_PIN_MASK : PORTB &= ~SER_PIN_MASK;
		PORTB |= SRCLK_PIN_MASK;
		PORTB &= ~SRCLK_PIN_MASK;
		byte <<= 1;
	}
	PORTB |= RCLK_PIN_MASK; 
	PORTB &= ~RCLK_PIN_MASK;
}

#define RS_PIN_MASK (1 << 2)
#define EN_PIN_MASK (1 << 3)
#define D4_PIN_MASK (1 << 4)
#define D5_PIN_MASK (1 << 5)
#define D6_PIN_MASK (1 << 6)
#define D7_PIN_MASK (1 << 7)

uint8_t LCDPORT = 0x00;

void lcd_command(uint8_t command) {
	LCDPORT = (LCDPORT & 0x0F) | (command & 0xF0);
	LCDPORT &= ~RS_PIN_MASK;
	LCDPORT |= EN_PIN_MASK;
	shift_byte(LCDPORT);
	_delay_us(1);
	LCDPORT &= ~EN_PIN_MASK;
	shift_byte(LCDPORT);
	_delay_us(200);
	LCDPORT = (LCDPORT & 0x0F) | (command << 4);
	LCDPORT |= EN_PIN_MASK;
	shift_byte(LCDPORT);
	_delay_us(1);
	LCDPORT &= ~EN_PIN_MASK;
	shift_byte(LCDPORT);
	_delay_ms(2);
}

void lcd_char(uint8_t ch) {
	LCDPORT = (LCDPORT & 0x0F) | (ch & 0xF0);
	LCDPORT |= RS_PIN_MASK;
	LCDPORT |= EN_PIN_MASK;
	shift_byte(LCDPORT);
	_delay_us(1);
	LCDPORT &= ~EN_PIN_MASK;
	shift_byte(LCDPORT);

	_delay_us(200);

	LCDPORT = (LCDPORT & 0x0F) | (ch << 4);
	LCDPORT |= EN_PIN_MASK;
	shift_byte(LCDPORT);
	_delay_us(1);
	LCDPORT &= ~EN_PIN_MASK;
	shift_byte(LCDPORT);
	_delay_ms(2);
}

void lcd_init() {
	_delay_ms(80);			// Delay > 80ms
	lcd_command(0x02);		// Use 4-bit mode
	lcd_command(0x28);      // 5 x 7, 2 line
	lcd_command(0x0c);      // Cursor off
	lcd_command(0x06);      // L-to-R
	_delay_ms(2);
}

void lcd_string(char* str, int max=16) {
	for(int i = 0; str[i] != 0 && i != max; ++i) {
		lcd_char(str[i]);
	}
}

void lcd_clear() {
	lcd_command(0x01);		// Clear
	_delay_ms(2);
	lcd_command(0x80);		// Cursor line 0, col 0
	_delay_ms(2);
}

char buffer0[17] = {0};
char buffer1[17] = {0};

uint8_t i2c_buffer[4] = {0};

#define DS3231_ADDR_READ 0b11010001
#define DS3231_ADDR_WRITE 0b11010000

#define MC4725_ADDR_READ 0b1111111
#define MC4725_ADDR_WRITE 0b1111110

void get_date_time() {
	start_i2c();
	transmit_i2c(DS3231_ADDR_WRITE);
	transmit_i2c(0x00); // Set start register
	stop_i2c();
	start_i2c();
	transmit_i2c(DS3231_ADDR_READ);
	i2c_buffer[0] = receive_i2c(0); // Seconds
	i2c_buffer[1] = receive_i2c(0); // Minutes
	i2c_buffer[2] = receive_i2c(1); // Hours
	stop_i2c();
}

// Set to first char second row lcd_command(0xC0);

uint8_t bcd_to_bin(uint8_t byte) {
	return 10 * ((byte & 0xF0) >> 4) + (byte & 0x0F);
}

void bcd_to_char(uint8_t value, char* buffer) {
	buffer[0] = (char)(0x30 + ((value & 0xF0) >> 4));
	buffer[1] = (char)(0x30 + (value & 0x0F));
}

uint8_t dec_to_bcd(uint8_t value) {
	return ((value / 10) << 4) + (value % 10);
}

void set_time(uint8_t hours, uint8_t minutes, uint8_t seconds) {
	start_i2c();
	transmit_i2c(DS3231_ADDR_WRITE);
	transmit_i2c(0x00);
	transmit_i2c(dec_to_bcd(seconds));
	transmit_i2c(dec_to_bcd(minutes));
	transmit_i2c(dec_to_bcd(hours));
	stop_i2c();	
}

void format_time(uint8_t offset) {
	static uint8_t cached_offset = 0;
	if(offset > 8) offset = 8;
	if(cached_offset != offset) {
		cached_offset = offset;
		for(int i = 0; i != offset; ++i) buffer0[i] = 0x20;
		buffer0[2+offset] = ':'; 
		buffer0[5+offset] = ':';
		for(int i = 8 - offset; i >= 0; --i) buffer0[offset+8+i] = 0x20;
	}
	bcd_to_char(i2c_buffer[0], &buffer0[6+offset]);
	bcd_to_char(i2c_buffer[1], &buffer0[3+offset]);
	bcd_to_char(i2c_buffer[2], &buffer0[offset]);
}

int main() {
	cli();
	CLKPR = 0x80;
	CLKPR = 0x00; // Set to run at 8 MHz
	sei();
	
	DDRB |= (SER_PIN_MASK | RCLK_PIN_MASK | SRCLK_PIN_MASK);
	
	lcd_init();
	setup_i2c();
	
	uint8_t offset = 4;
	uint8_t curr_min = 0;
	uint8_t dir = 1;
	uint16_t value = 0;
	
	while(true) {
		lcd_command(0x80);
		get_date_time();
		if(curr_min != i2c_buffer[1]) {
			curr_min = i2c_buffer[1];
			offset = dir ? offset + 1 : offset - 1;
			if(offset == 8) dir = 0;
			else if(offset == 0) dir = 1;
		}
		format_time(offset);
		lcd_string(buffer0);
		
		lcd_command(0xC0);
		sine_wave(&value);
		itoa(int(value), buffer1, 10);
		// Get rid of null
		for(int i = 0; i != 4; ++i) {
			if(buffer1[i] == 0) buffer1[i] = 0x20;
		}
		lcd_string(buffer1);		
	}
	
	return 0;
}

