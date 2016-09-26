#include <io.h> // inb, printc_xy
#include <interrupt.h> // char_map
#include <zeos_interrupt.h> // zeos_show_clock

/******************************************/
/* Service Routines for Interrupts        */
/******************************************/

void rsi_keyboard() {
	unsigned char port = inb(0x60);

	// b0..6 Scan code
	// b7 Make/Break (press/release)
	unsigned char isKeyPressed = !(port & 0x80);
	unsigned int key_code = port & 0x7F;

	if (isKeyPressed) {
		char character = char_map[key_code];

		if (character == '\0')
			character = 'C';

		
		unsigned char x = 0x00;
		unsigned char y = 0x00;
		printc_xy(x, y, character);
	}
}

void rsi_clock() {
	zeos_show_clock();
}
