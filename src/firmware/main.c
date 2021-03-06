/*
 * Keyboard_Firmware.c
 *
 * Author : Ryan Mah
 */

#ifndef F_CPU
#define F_CPU 1000000UL
#endif

#define USART_BAUDRATE 9600UL
#define UBRR_VALUE (((F_CPU / (USART_BAUDRATE * 8UL))) - 1)

#include <avr/io.h>
#include <stdbool.h>
#include <util/delay.h>

#define Bit_is_Set(byte, bit) (byte & (1 << bit))
#define Bit_is_Clear(byte, bit) !(byte & (1 << bit))


///////////// FUNCTION DECLARATIONS /////////////
void pins_init();
void uart_init();
int main();

void clear_leds();
void write_led(int num);
void blink_led(int led_num);
void tx_uart(char data);
void send_keystrokes(int curr_keys[4][4]);

void init();

void set_col_low(int column);
void set_col_high(int column);

int scan_rows();

void scan();

void populate_prev_keys(int prev_keys[4][4], int curr_keys[4][4]);
int has_repeated_keystrokes(int prev_keys[4][4], int curr_keys[4][4]);
int is_holding_key(int prev_keys[4][4], int curr_keys[4][4]);



int main();

///////////// FUNCTION DEFINITIONS /////////////

/*
ATmega has the following possible states for digital pins:

DDRX[8:0]	PORTX[8:0]		I/O		Pin State
   0           0	       Input	  Hi-Z
   0           1		   Input    Pulled-up
   1           0           Output     Low
   1           1           Output     High

*/

// LED functions for easy debugging
void clear_leds() {
	PORTB = 0;
}

void write_led(int num) {
	if ( (num >> 3) & 1 ) {
		num &= 0b11110111;
		num |= 0b01000000;
	}
	PORTB = num;
}

void blink_led(int led_num) {
	int led_map[4] = {0, 1, 2, 6};

	PORTB |= (1 << led_map[led_num]);
	_delay_ms(500);
	PORTB &= ~(1 << led_map[led_num]);
	_delay_ms(500);

}

void tx_uart(char data) {
	// wait for transmit buffer to be empty
	while(!(UCSR0A & (1 << UDRE0)));

	// load data into transmit register
	UDR0 = data;
}


void pins_init(){
	int led_pins[4] = {0, 1, 2, 6};
	for (int i = 0; i < 4; i++) {
		DDRB |= (1 << led_pins[i]);
	}

	// init rows
	int row_pins[4] = {0, 1, 2, 3};
	for (int i = 0; i < 4; i++) {
		DDRC &= ~(1 << row_pins[i]);
		PORTC |= (1 << row_pins[i]);
	}

	// init columns
	int col_pins[4] = {2, 3, 4, 5};
	for (int i = 0; i < 4; i++) {
		DDRD &= ~(1 << col_pins[i]);
		PORTD &= ~(1 << col_pins[i]);
	}

}

void uart_init() {
	// UART config
	UCSR0A |= (1 << U2X0);	// double transmission speed

	UBRR0H = (UBRR_VALUE >> 8) & 0xFF;
	UBRR0L = UBRR_VALUE & 0xFF;

	UCSR0B |= (1 << TXEN0);	// enable tx
	UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00);	// 8 data bits, 1 stop bit
}


void set_col_low(int column) {
	DDRD |= ((1 << 2) << column);
	PORTD &= ~((1 << 2) << column);
}

void set_col_high(int column) {
	DDRD |= ((1 << 2) << column);
	PORTD |= ((1 << 2) << column);
}


int scan_rows() {
	// returns the status of each row as a bit in an int, nth bit is nth row
	int foo = ~PINC;
	return (foo & 0xF);
}

void scan(int keys[4][4]) {
	// returns an integer who's 0th to 3rd bits represent the state of the 0th to 3rd row
	// keys is a 2-D array that is indexed "keys[row][col]" since that's the most convient way to do it with an array
	pins_init();

	for (int col = 0; col < 4; col++) {
		// Scan columns
		_delay_ms(15);	// debounce time
		set_col_low(col);

		int rows_status = scan_rows();
		//write_led(rows_status);

		for (int row = 0; row < 4; row++) {
			if Bit_is_Set(rows_status, row) {
				keys[row][col] = 1;
			}
			else {
				keys[row][col] = 0;
			}
		}

		set_col_high(col);
	}
}



void populate_prev_keys(int prev_keys[4][4], int curr_keys[4][4]) {
	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 4; col++) {
			prev_keys[row][col] = curr_keys[row][col];
		}
	}
}

int has_repeated_keystrokes(int prev_keys[4][4], int curr_keys[4][4]) {
	// returns 1 if a keystroke is repeated, 0 otherwise
	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 4; col++) {
			if ( (curr_keys[row][col] == 1) && (curr_keys[row][col] == prev_keys[row][col]) ) {
				return 1;
			}
		}
	}
	return 0;
}

int is_holding_key(int prev_keys[4][4], int curr_keys[4][4]) {
	// waits for 5 repeated keystrokes to register that the user is holding the key

	for (int i = 0; i < 3; i++) {
		populate_prev_keys(prev_keys, curr_keys);
		scan(curr_keys);

		if (has_repeated_keystrokes(prev_keys, curr_keys) == 0) {return 0;}

	}

	return 1;
}


void send_keystrokes(int curr_keys[4][4]) {
	// will be used to convert the status of keys to an actual value
	uart_init();

	int led_keys_lookup[4][4] = {
		{15, 14, 13, 12},
		{11, 10, 9, 8},
		{7, 6, 5, 4},
		{3, 2, 1, 0}
	};
	int usb_keys_lookup[4][4] = {
		{'.', '0', '\n', '3'},
		{'2', '1', '6', '5'},
		{'4', '+', '9', '8'},
		{'7', '-', '*', '/'}
	};

	for (int row = 3; row >= 0; row--) {

		for (int col = 3; col >= 0; col--) {

			if (curr_keys[row][col] == 1) {
				write_led(led_keys_lookup[row][col]);
				_delay_ms(25);
				clear_leds();
				_delay_ms(25);
				tx_uart(usb_keys_lookup[row][col]);
			}
		}

	}

}



int main() {
	// flash all LEDs on startup to indicate everything is working as intended
	pins_init();
	write_led(0xF);
	_delay_ms(1000);
	clear_leds();
	_delay_ms(100);

	// will be used to keep track of the keys that are pressed
	int curr_keys[4][4] = {
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0}
	};
	int prev_keys[4][4] = {
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0}
	};

    while (1) {
		populate_prev_keys(prev_keys, curr_keys);	// update the previously pressed keys

		scan(curr_keys);	// scan keypad

		int has_repeats = has_repeated_keystrokes(prev_keys, curr_keys);
		if (has_repeats == 0) {
			// if there are no repeated keystrokes, send the keystrokes
			send_keystrokes(curr_keys);
		}

		else if (is_holding_key(prev_keys, curr_keys) == 1) {
			// this if statement handles the case where the user is holding the key down
			// try holding down a key on your keyboard, and this is close to what happens

			_delay_ms(250);		// wait for a bit to confirm that they're holding it
			while (is_holding_key(prev_keys, curr_keys) == 1) {
				// while they're still holding the key, keep sending the keystroke
				send_keystrokes(curr_keys);
			}
		}


	}
}
