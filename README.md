ATtiny I2C
==========

A bit-banged i2c implementation to be ported to USI hardware on the ATtiny.

Master ATTiny outputs incrementing bytes on i2c to slave ATTiny which displays digit on 74HC595 shift register.

I2C bus uses 4k7 pull-up resistors to 5v Vcc and open-drain on SDA/SCL pins.
