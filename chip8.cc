#include "chip8.h"



#define not_handled(op)\
	printf("\nUnrecognized instruction: %04x\n", op); exit(2);


mt19937 rnd{};


/**
 * Load ROM from file into memory
 */
bool Chip8::load_program(string file) {
	FILE *f = fopen(file.c_str(), "rb");

	if (f == NULL) {    
		printf("error: Couldn't open '%s'\n", file.c_str());    
		return false;
	}

	// get file size
	fseek(f, 0L, SEEK_END);
	program_size = ftell(f);
	fseek(f, 0L, SEEK_SET);

	// allocate memory for ROM (buffer)
	byte* rom = (byte*) malloc(sizeof(byte) * program_size);
	if (rom == NULL) {
		cerr << "Couldn't allocate memory for ROM" << endl;
		return false;
	}

	// read program into buffer
	fread(rom, sizeof(byte), program_size, f);

	// load program into memory
	for (int i=PC, j=0; i < int(PC + program_size); ++i) {
		memory[i] = (byte) rom[j++];
	}

	free(rom);
	fclose(f);

	return true;
}



/**
 * Intepreter
 *
 * inteprete an instruction
 *  a single cycle
 *
 * See chip8 instruction set at http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#3.1
 */
void Chip8::emulate_op() {
	byte* op = &memory[PC];
	byte msb = *op, lsb = *(op + 1);

	// increment program counter
	//  point to next instruction
	PC += 2;

	int tmp;
	const byte nnn = ((msb & 0xf) << 8) | lsb; // address (from opcode)

	// 0x_xkk
	// 0x_xyk
	// 0x_xyn
	// _ => 0-F
	const byte x = msb&0xf, kk = lsb, y = kk >> 4, n = kk&0xf;

	switch (msb >> 4) {

		// 0x0e-
		case 0x0: {
			switch (lsb) {
				// 0x0e0
				case 0xe0: // clr
					memset(screen, 0, 2048);
					break;

				// 0x0ee
				case 0xee: // ret
					PC = stack[--sp];
					break;

				default:
					not_handled(lsb);
			}
		}
		break;

		// 0x1nnn
		case 0x1: // jmp nnn
			PC = nnn;
			break;

		// 0x2nnn
		case 0x2: // call nnn
			stack[sp++] = PC;
			PC = nnn;
			break;

		// 03xkk
		case 0x3: // jeq Vx, Vkk
			if (V[x] == kk) PC += 2;
			break;

		// 0x4xkk
		case 0x4: //jneq Vx, Vkk
			if (V[x] != kk) PC += 2;
			break;

		// 0x5xy0
		case 0x5: // jeqr Vx, Vy
			if (V[x] == V[y]) PC += 2;
			break;

		// 0x6xkk
		case 0x6: // mov Vx, kk
			V[x] = kk;
			break;

		// 0x7xkk
		case 0x7: // add Vx, Vkk
			V[x] += V[kk];
			break;

		// 0x8xyn
		case 0x8: {
			switch (lsb & 0xf) {
				// 0x8xy0
				case 0x0: // mov Vx, Vy
					V[x] = V[y];
					break;

				// 0x8xy1
				case 0x1: // or Vx, Vy
					V[x] |= V[y];
					break;

				// 0x8xy2
				case 0x2: // and Vx, Vy
					V[x] = V[x] & V[y];
					break;

				// 0x8xy3
				case 0x3: // xor Vx, Vy
					V[x] = V[x] ^ V[y];
					break;

				// 0x8xy4
				case 0x4: // addr Vx, Vy
					tmp = V[x] + V[y];
					V[0xF] = (tmp > 0xff); // VF (carry)
					V[x] += V[y];
					break;

				// 0x8xy5
				case 0x5: // sub Vx, Vy
					V[0xF] = V[x] > V[y];
					V[x] -= V[y];
					break;

				// 0x8xy6
				case 0x6: // shr Vx
					V[0xF] = V[x]&1;
					V[x] >>= 1;
					break;

				// 0x8xy7
				case 0x7: // subb Vy, Vx
					V[0xF] = V[y] > V[x];
					V[y] -= V[x];
					break;

				// 0x8ye
				case 0xe: // shl Vx
					V[0xF] = (V[x]&(1<<7)) != 0;
					V[x] <<= 1;
					break;

				default:
					not_handled(lsb);
			}
		}
		break;

		// 9xy0
		case 0x9: // jneqr Vx, Vy
			if (V[x] != V[y]) PC += 2;
			break;

		// Annn
		case 0xA: // mov I, [nnn]
			I = nnn;
			break;

		// 0xBnnn
		case 0xB: // jmp V0+nnn
			PC = V[0x0] + nnn;
			break;

		// 0xCxkk
		case 0xC: // rnd Vx, kk
			V[x] = uniform_int_distribution<>(0, 255)(rnd) & kk;
			break;


		// TODO
		// -----
		// 0xDxyn
		case 0xD: // draw Vx, Vy, n
		break;


		// 0xEx--
		case 0xE: {
			switch (lsb) {
				// 0xE9E
				case 0x9E: // jkey Vx
					if (key_pressed[V[x]]) PC += 2;
					break;

				// 0xA1
				case 0xA1: // jnkey Vx
					if (!key_pressed[V[x]]) PC += 2;
					break;
			}
		}
		break;

		// 0xFx--
		case 0xF: {
			switch (lsb) {
				// 0xFx07
				case 0x07: // getdelay Vx
					V[x] = DT;
					break;

				// 0xFx0A
				case 0x0A: { // waitkey Vx
					bool p = 0;
					for (int i = 0x0; i <= 0xF; ++i) {
						if (key_pressed[i]) {
							V[x]=i, p=1;
							break;
						}
					}

					if (!p) return;
					break;
				}

				// 0xFx15
				case 0x15: // setdelay Vx
					DT = V[x];
					break;

				// 0xFx18
				case 0x18: // setsound Vx
					ST = V[x];
					break;

				// 0xFx1E
				case 0x1E: // mov I, Vx
					I += V[x];
					break;

				// 0xFx29
				case 0x29: // spritei I, Vx
					I = 5 * V[x];
					break;

				// 0xFx33
				case 0x33: // bcd [I], Vx
					tmp = V[x];
					byte a,b,c;
					c = tmp%10, tmp/=10;
					b = tmp%10, tmp/=10;
					a = tmp;

					memory[I] = a, memory[I+1] = b, memory[I+2] = c;
					break;

				// 0xFx55
				case 0x55: // mov [I], V0-VF
					for (int i = 0; i <= x; ++i)
						memory[I+i] = V[i];
					break;

				// 0xFx65
				case 0x65: // mov V0-VF, [I]
					for (int i = 0; i <= x; ++i)
						V[i] = memory[I+i];
					break;

				default:
					not_handled(lsb);
			}
			break;
		}

		default:
			not_handled(msb);
	}

	// Time & Sound

	// delay timer
	if (DT > 0)
		DT -= 1;

	// sound timer
	if (ST > 0) {
		// sound: beep
		ST -= 1;
	}
}



/*


Dxyn - DRW Vx, Vy, nibble
Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.

The interpreter reads n bytes from memory, starting at the address stored in I. These bytes are then displayed as sprites on screen at coordinates (Vx, Vy).
Sprites are XORed onto the existing screen. If this causes any pixels to be erased, VF is set to 1, otherwise it is set to 0.
If the sprite is positioned so part of it is outside the coordinates of the display, it wraps around to the opposite side of the screen.

See instruction 8xy3 for more information on XOR, and section 2.4, Display, for more information on the Chip-8 screen and sprites.



*/


