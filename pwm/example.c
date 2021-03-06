/* Simple example for Teensy USB Development Board
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2008 PJRC.COM, LLC
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* Copyright (c) 2009 Mark Gross */
/* sublicenced under the Apache License */


#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>
#include "usb_serial.h"
#define pchar(c) usb_serial_putchar(c)


#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))

//void send_str(const char *s);
//void send_str_ram(const char *s);
//void phex16(unsigned int i);
////void phex(unsigned char c);
//uint8_t recv_str(char *buf, uint8_t size);
//void parse_and_execute_command(const char *buf, uint8_t num);


void phex1(unsigned char c)
{
	usb_serial_putchar(c + ((c < 10) ? '0' : 'A' - 10));
}

void phex(unsigned char c)
{
	phex1(c >> 4);
	phex1(c & 15);
}

void phex16(unsigned int i)
{
	phex(i >> 8);
	phex(i);
}

void send_str_ram(const char *s)
{
	char c;
	while (1) {
		c = *(s++);
		if (!c) break;
		usb_serial_putchar(c);
	}
}
// Send a string to the USB serial port.  The string must be in
// flash memory, using PSTR
//
void send_str(const char *s)
{
	char c;
	while (1) {
		c = pgm_read_byte(s++);
		if (!c) break;
		usb_serial_putchar(c);
	}
}

// Receive a string from the USB serial port.  The string is stored
// in the buffer and this function will not exceed the buffer size.
// A carriage return or newline completes the string, and is not
// stored into the buffer.
// The return value is the number of characters received, or 255 if
// the virtual serial connection was closed while waiting.
//
uint8_t recv_str(char *buf, uint8_t size)
{
	int16_t r;
	uint8_t count=0;

	while (count < size) {
		r = usb_serial_getchar();
		if (r != -1) {
			if (r == '\r' || r == '\n') return count;
			if (r >= ' ' && r <= '~') {
				*buf++ = r;
				usb_serial_putchar(r);
				count++;
			}
		} else {
			if (!usb_configured() ||
			  !(usb_serial_get_control() & USB_SERIAL_DTR)) {
				// user no longer connected
				return 255;
			}
			// just a normal timeout, keep waiting
		}
	}
	return count;
}

// parse a user command and execute it, or print an error message
//
void parse_and_execute_command(const char *buf, uint8_t num)
{

	if (buf[0] == '+'){
		OCR1C ++;
		OCR1B ++;
		phex(OCR1C);
	}

	if (buf[0] == '-'){
		OCR1C --;
		OCR1B --;
		phex(OCR1C);
	}

	if (buf[0] == '1'){
		PORTD &= ~(1<<0);
		PORTD |= (1<<1);
	}
	if (buf[0] == '2'){
		PORTD |= (1<<0);
		PORTD &= ~(1<<1);
	}
	if (buf[0] == '9'){
		PORTD &= ~(1<<2);
		PORTD |= (1<<3);
	}
	if (buf[0] == '0'){
		PORTD |= (1<<2);
		PORTD &= ~(1<<3);
	}
}


void init_pwm(void)
{
  DDRB = (1<<6)| (1<<7);
  TCCR1A = _BV(WGM10);			/* Fast PWM, 0xFF is top */
  TCCR1B = _BV(WGM12) | _BV(CS11);	/* div 8 clock prescaler */
  
  OCR1A = 0;
  OCR1B = 0;
  TIMSK1 &= ~( _BV(OCIE1A) | _BV(OCIE1B) | _BV(OCIE1C) | _BV(TOIE1) );

  TCCR1A &= 0x03;
  TCCR1A |= 0xA8;
  //TCCR1A = TCCR1A & (_BV(COM1A0) | _BV(COM1B0) | _BV(COM1A1) | _BV(COM1B1));
}



int main(void)
{
	char buf[32];
	uint8_t n;

	// set for 16 MHz clock, and turn on the LED
	CPU_PRESCALE(0);
  	DDRD = 0xFF;
	PORTD = 0xFA;
	init_pwm();

	// initialize the USB, and then wait for the host
	// to set configuration.  If the Teensy is powered
	// without a PC connected to the USB port, this 
	// will wait forever.
	usb_init();
	while (!usb_configured()) /* wait */ ;
	_delay_ms(3000);

	while (1) {
		// wait for the user to run their terminal emulator program
		// which sets DTR to indicate it is ready to receive.
		while (!(usb_serial_get_control() & USB_SERIAL_DTR)) /* wait */ ;

		// discard anything that was received prior.  Sometimes the
		// operating system or other software will send a modem
		// "AT command", which can still be buffered.
		usb_serial_flush_input();

		// print a nice welcome message
		send_str(PSTR("\r\nline PWM test "
			"Commands\r\n"
			"  +/-   increase / decrease duty cycle\r\n"
			"  1 Right Forward\r\n"
			"  2 Right Reverse\r\n"
			"  9 Left Reverse\r\n"
			"  0 Left Reverse\r\n"));

		// and then listen for commands and process them
		while (1) {
			send_str(PSTR("> "));
			n = recv_str(buf, 1);
			//n = recv_str(buf, sizeof(buf));
			if (n == 255) break;
			send_str(PSTR("\r\n"));
			parse_and_execute_command(buf, n);
			//memset(buf,0,32);
			send_str(PSTR("\r\n"));
		}
	}
}


