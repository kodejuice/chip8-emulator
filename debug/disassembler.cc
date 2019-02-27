#include "../chip8.h"

/*

Converts program from memory back to Chip8 instructions.

Chip8 doesnt have an assembly instruction set,
so i'm just going to use something similar to NASM.

The generated code isnt neccessarily valid, 
its just used for debugging purpose.

*/


string decode (int, byte, byte);


string Chip8::disassemble() {

	byte lsb, msb;
	string str = "";

	for (int i=PC; i < (PC + program_size); i+=2) {
		msb = memory[i], lsb = memory[i+1];

		str += decode(i, msb, lsb) + "\n";
	}

	return str;
}



#define ADDR ((msb&0xf)<<8)|lsb // address
// 0xf = 0b1111


// decode instruction
string decode(int i, byte msb, byte lsb) {
	char ln[20];
	sprintf(ln, "%04x:  %02x %02x  =>  ", i, msb, lsb);
	string str = ln;

	byte nib = msb >> 4;

	switch (nib)
	{
		case 0x0: 
			switch (lsb)
			{
				case 0xe0: {sprintf(ln,"%s","clear"), str+=ln;} break;
				case 0xee: {sprintf(ln,"%s","ret"), str+=ln;} break;
				default: {sprintf(ln,"unknown"), str+=ln;} break;
			}
			break;
		case 0x1: {sprintf(ln,"%s 0x%03x", "jmp", ADDR), str+=ln;} break;
		case 0x2: {sprintf(ln,"%s 0x%02x", "call", ADDR), str+=ln;} break;
		case 0x3: {sprintf(ln,"%s V%01X, 0x%02x", "jeq", msb&0xf, lsb), str+=ln;} break;
		case 0x4: {sprintf(ln,"%s V%01X, 0x%02x", "jneq", msb&0xf, lsb), str+=ln;} break;
		case 0x5: {sprintf(ln,"%s V%01X, V%01X", "jeqr", msb&0xf, lsb>>4), str+=ln;} break;
		case 0x6: {sprintf(ln,"%s V%01X, 0x%02x", "mov", msb&0xf, lsb), str+=ln;} break;
		case 0x7: {sprintf(ln,"%s V%01X, 0x%02x", "add", msb&0xf, lsb), str+=ln;} break;
		case 0x8:
			{
				byte lastnib = lsb&0xf;
				switch(lastnib)
				{
					case 0: {sprintf(ln,"%s V%01X, V%01X", "mov", msb&0xf, lsb>>4), str+=ln;} break;
					case 1: {sprintf(ln,"%s V%01X, V%01X", "or", msb&0xf, lsb>>4), str+=ln;} break;
					case 2: {sprintf(ln,"%s V%01X, V%01X", "and", msb&0xf, lsb>>4), str+=ln;} break;
					case 3: {sprintf(ln,"%s V%01X, V%01X", "xor", msb&0xf, lsb>>4), str+=ln;} break;
					case 4: {sprintf(ln,"%s V%01X, V%01X", "addr", msb&0xf, lsb>>4), str+=ln;} break;
					case 5: {sprintf(ln,"%s V%01X, V%01X", "sub", msb&0xf, lsb>>4), str+=ln;} break;
					case 6: {sprintf(ln,"%s V%01X, V%01X", "shr", msb&0xf, lsb>>4), str+=ln;} break;
					case 7: {sprintf(ln,"%s V%01X, V%01X", "subb", msb&0xf, lsb>>4), str+=ln;} break;
					case 0xe: {sprintf(ln,"%s V%01X, V%01X", "shl", msb&0xf, lsb>>4), str+=ln;} break;
					default: {sprintf(ln,"unknown"), str+=ln;} break;
				}
			}
			break;
		case 0x9: {sprintf(ln,"%s V%01X, V%01X", "jneqr", msb&0xf, lsb>>4), str+=ln;} break;
		case 0xa: {sprintf(ln,"%s I, [0x%03x]", "mov", ADDR), str+=ln;} break;
		case 0xb: {sprintf(ln,"%s 0x%03x+(V0)", "jmp", ADDR), str+=ln;} break;
		case 0xc: {sprintf(ln,"%s V%01X, 0x%02X", "rand", msb&0xf, lsb), str+=ln;} break;
		case 0xd: {sprintf(ln,"%s V%01X, V%01X, 0x%01x", "draw", msb&0xf, lsb>>4, lsb&0xf), str+=ln;} break;
		case 0xe: 
			switch(lsb)
			{
				case 0x9E: {sprintf(ln,"%s V%01X", "jkey", msb&0xf), str+=ln;} break;
				case 0xA1: {sprintf(ln,"%s V%01X", "jnkey", msb&0xf), str+=ln;} break;
				default: {sprintf(ln,"unknown"), str+=ln;} break;
			}
			break;
		case 0xf: 
			switch(lsb)
			{
				case 0x07: {sprintf(ln,"%s V%01X", "getdelay", msb&0xf), str+=ln;} break;
				case 0x0a: {sprintf(ln,"%s V%01X", "waitkey", msb&0xf), str+=ln;} break;
				case 0x15: {sprintf(ln,"%s V%01X", "setdelay", msb&0xf), str+=ln;} break;
				case 0x18: {sprintf(ln,"%s V%01X", "setsound", msb&0xf), str+=ln;} break;
				case 0x1e: {sprintf(ln,"%s I, V%01X", "mov", msb&0xf), str+=ln;} break;
				case 0x29: {sprintf(ln,"%s I, V%01X", "spritei", msb&0xf), str+=ln;} break;
				case 0x33: {sprintf(ln,"%s [I], V%01X", "bcd", msb&0xf), str+=ln;} break;
				case 0x55: {sprintf(ln,"%s [I], V0-V%01X", "mov", msb&0xf), str+=ln;} break;
				case 0x65: {sprintf(ln,"%s V0-V%01X, [I]", "mov", msb&0xf), str+=ln;} break;
				default: {sprintf(ln,"unknown"), str+=ln;} break;
			}
			break;
	}

	return str;
}


