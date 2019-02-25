#include "chip8.h"



#define not_handled(op)\
	printf("\nUnknown instruction: %04x\n", op); exit(3);


/**
 * Load ROM from file into memory
 */

bool Chip8::loadROM(string file) {
	FILE *f = fopen(file.c_str(), "rb");

	if (f == NULL) {    
		printf("error: Couldn't open '%s'\n", file.c_str());    
		return false;
	}

	// Get file size
	fseek(f, 0L, SEEK_END);
	program_size = ftell(f);
	fseek(f, 0L, SEEK_SET);

	// size check
	if (program_size >= 3584) {
		cerr << "ROM too large, (max allowed = 3.5KB)" << endl;
		return false;
	}

	// Allocate memory for ROM
	byte* rom = (byte*) malloc(sizeof(byte) * program_size);
	if (rom == NULL) {
		cerr << "Couldn't allocate memory for ROM" << endl;
		return false;
	}

	size_t read_size = fread(rom, sizeof(byte), program_size, f);
	if (read_size != program_size) {
		cerr << "Couldn't read ROM" << endl;
		return false;
	}

	// load ROM into memory (RAM)
	for (int i=pc, j=0; i < int(pc + program_size); ++i) {
		memory[i] = (byte) rom[j++];
	}

	free(rom);
	fclose(f);

	return true;
}



/**
 * Emulate an instruction
 */

void Chip8::emulate_op() {
	byte* op = &memory[pc];
	byte msb = *op, lsb = *(op + 1);

	const int nnn = ((msb&0xf)<<8)|lsb; // address

	int x = msb&0xf, kk = lsb, y = kk >> 4;

	switch (msb >> 4) {

		// 0x0E
		case 0x0: {
			switch (lsb) {
				case 0xe0: // clr
					memset(screen, 0, 2048);
					pc += 2;
					break;

				case 0xee: // ret
					pc = stack[--sp];
					pc += 2;
					break;

				default:
					not_handled(lsb);
			}
		}
		break;

		// 0x1nnn
		case 0x1: // jmp
			pc = nnn;
			break;

		// 0x2nnn
		case 0x2: // call
			stack[sp++] = pc;
			pc = nnn;
			break;

		// 03xkk
		case 0x3: // jeq
			if (V[x] == kk) pc += 2;
			pc += 2;
			break;

		// 0x4xkk
		case 0x4: //jneq
			if (V[x] != kk) pc += 2;
			pc += 2;
			break;

		// 0x5xy0
		case 0x5: // jeqr
			if (V[x] == V[y]) pc += 2;
			pc += 2;
			break;

		// 0x6xkk
		case 0x6: // mov
			V[x] = kk;
			pc += 2;
			break;

		// 0x7xkk
		case 0x7: // add
			V[x] += V[kk];
			pc += 2;
			break;

		// 0x8
		case 0x8: {
			switch (lsb & 0xf) {
				case 0x0: // mov
					V[x] = V[y];
					pc += 2;
					break;

				case 0x1: // or
					V[x] |= V[y];
					pc += 2;
					break;

				case 0x2: // and
					V[x] = V[x] & V[y];
					pc += 2;
					break;

				case 0x3: // xor
					V[x] = V[x] ^ V[y];
					pc += 2;
					break;
			}
		}
		break;


	}
}


