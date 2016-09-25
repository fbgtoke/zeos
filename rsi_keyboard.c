#include <io.h> // inb, printc_xy
#include <interrupt.h> // char_map

/******************************************/
/* Service Routine for Keyboard Interrupt */
/*                                        */
/* Read keypress and print to screen      */
/******************************************/

void rsi_keyboard() {
	unsigned char port = inb(0x60);

	// b0..6 Scan code
	// b7 Make/Break (press/release)
	unsigned char isKeyPressed = !(port & 0x8);
	unsigned int key_code = port & 0x7;

	if (isKeyPressed) {
		char character;

		if (key_code > 99)
			character = 'C';
		else
			character = char_map[key_code];

		
		unsigned char x = 0x00;
		unsigned char y = 0x00;
		printc_xy(x, y, character);
	}
}
