/*
 * i2c_lcd.cpp
 *
 * Created: 2022/04/16 09:14:22
 * Author : otgaard
 */ 

#include "i2c_master.h"

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
	_delay_ms(20);			// LCD Power ON delay always >15ms	
	lcd_command(0x02);		// Use 4-bit mode
	lcd_command(0x28);      // 5 x 7, 2 line
	lcd_command(0x0c);      // Cursor off
	lcd_command(0x06);      // L-to-R
	_delay_ms(2);
}

void lcd_string(char* str) {
	for(int i = 0; str[i] != 0; ++i) {
		lcd_char(str[i]);
	}
}

void lcd_clear() {
	lcd_command(0x01);		// Clear
	_delay_ms(2);
	lcd_command(0x80);		// Cursor line 0, col 0
	_delay_ms(2);
}

char buffer0[16] = {' '};
char buffer1[16] = {' '};

uint8_t i2c_buffer[4] = {0};
uint8_t i2c_copy[4] = {0};

#define DS3231_ADDR_READ 0b11010001
#define DS3231_ADDR_WRITE 0b11010000

void getDateTime() {
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

uint8_t bcd_hours_to_bin(uint8_t byte) {
	return 10 * ((byte & 0x03) >> 4) + (byte & 0x0F);
}

void bcd_to_char(uint8_t value, char* buffer) {
	buffer[0] = (char)(0x30 + ((value & 0xF0) >> 4));
	buffer[1] = (char)(0x30 + (value & 0x0F));
}

uint8_t dec_to_bcd(uint8_t value) {
	return ((value / 10) << 4) + (value % 10);
}

void setTime(uint8_t hours, uint8_t minutes, uint8_t seconds) {
	start_i2c();
	transmit_i2c(DS3231_ADDR_WRITE);
	transmit_i2c(0x00);
	transmit_i2c(dec_to_bcd(seconds));
	transmit_i2c(dec_to_bcd(minutes));
	transmit_i2c(dec_to_bcd(hours));
	stop_i2c();	
}

int main() {
	cli();
	CLKPR = 0x80;
	CLKPR = 0x00; // Set to run at 8 MHz
	sei();
	
	DDRB |= (SER_PIN_MASK | RCLK_PIN_MASK | SRCLK_PIN_MASK);
	
	lcd_init();
	setup_i2c();
	
	//setTime(16, 38, 00);	
	
	while(true) {
		lcd_clear();
		getDateTime();
		bcd_to_char(i2c_buffer[0], &buffer0[6]);
		buffer0[2] = ':';
		bcd_to_char(i2c_buffer[1], &(buffer0[3]));
		buffer0[5] = ':';
		bcd_to_char(i2c_buffer[2], &(buffer0[0]));
		buffer0[9] = 0;
		lcd_string(buffer0);
		_delay_ms(200);
	}
	
	return 0;
}

