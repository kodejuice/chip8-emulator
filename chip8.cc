/**
 * Chip8 Emulator
 * 
 * chip8.cc
 * 
 * 	Contains code to load a program,
 *   and the instruction interpreter.
 *
 * (C) Sochima Biereagu, 2019
*/

#include "chip8.h"

mt19937 rnd{};


/**
 * Load ROM into memory
 */
bool Chip8::load_program(string file) {
	FILE *f = fopen(file.c_str(), "rb");

	if (f == NULL) 
		return printf("Error: Couldn't open '%s'\n", file.c_str()), false;

	// get file size
	fseek(f, 0L, SEEK_END);
	program_size = ftell(f);
	fseek(f, 0L, SEEK_SET);

	// allocate memory for ROM (buffer)
	byte* rom = (byte*) malloc(sizeof(byte) * program_size);

	// read program into buffer
	fread(rom, sizeof(byte), program_size, f);

	// load program from buffer into memory
	for (int i=PC, j=0; i < int(PC + program_size); ++i, ++j) {
		memory[i] = rom[j];
	}

	free(rom);
	fclose(f);

	return true;
}



/**
 * Interprete an instruction
 *  a single cycle
 *
 * See chip8 instruction set at http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#3.1
 */
void Chip8::emulate_op() {
	#define not_handled(m,l)\
		printf("\nUnrecognized instruction: %04x %04x\n", m,l); exit(2);

	int tmp;
	int opcode = (memory[PC] << 8) | memory[PC+1];
	int msb = opcode>>8, lsb = opcode&0xff;

    printf("(%x) %x %x | pc = %x\n", opcode, memory[PC], memory[PC+1], PC);

	// Get bit-fields from instruction/opcode
	int u   = (opcode>>12) & 0xF,
		x   = (opcode>>8) & 0xF,
		y   = (opcode>>4) & 0xF,
		kk  = (opcode) & 0xFF,
		n   = (opcode) & 0xF,
		nnn = (opcode) & 0xFFF;

	// alliases for common registers
	byte &Vx = V[x], &Vy = V[y], &VF = V[0xF];

	switch (msb >> 4) {
		// 0x0e-
		case 0x0: {
			switch (lsb&0xf) {
				// 0x00e0
				case 0x0: // clr
					memset(screen, 0, 2048);
					redraw = true;
					PC += 2;
					break;

				// 0x00ee
				case 0xe: // ret
					PC = stack[--sp] + 2;
					break;

				default:
					not_handled(msb, lsb);
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
			if (Vx == kk) PC += 2;
			PC += 2;
			break;

		// 0x4xkk
		case 0x4: //jneq Vx, Vkk
			if (Vx != kk) PC += 2;
			PC += 2;
			break;

		// 0x5xy0
		case 0x5: // jeqr Vx, Vy
			if (Vx == Vy) PC += 2;
			PC += 2;
			break;

		// 0x6xkk
		case 0x6: // mov Vx, kk
			Vx = kk;
			PC += 2;
			break;

		// 0x7xkk
		case 0x7: // add Vx, kk
            Vx += kk;
			PC += 2;
			break;

		// 0x8xyn
		case 0x8: {
			switch (lsb & 0xf) {
				// 0x8xy0
				case 0x0: // mov Vx, Vy
					Vx = Vy;
					PC += 2;
					break;

				// 0x8xy1
				case 0x1: // or Vx, Vy
					Vx |= Vy;
					PC += 2;
					break;

				// 0x8xy2
				case 0x2: // and Vx, Vy
					Vx &= Vy;
					PC += 2;
					break;

				// 0x8xy3
				case 0x3: // xor Vx, Vy
					Vx ^= Vy;
					PC += 2;
					break;

				// 0x8xy4
				case 0x4: // addr Vx, Vy
					tmp = Vx + Vy;
					VF = (tmp > 0xff);
					Vx += Vy;
					PC += 2;
					break;

				// 0x8xy5
				case 0x5: // sub Vx, Vy
					VF = Vx > Vy;
					Vx -= Vy;
					PC += 2;
					break;

				// 0x8xy6
				case 0x6: // shr Vx
					VF = Vx&1;
					Vx >>= 1;
					PC += 2;
					break;

				// 0x8xy7
				case 0x7: // subb Vy, Vx
					VF = Vy > Vx;
					Vy -= Vx;
					PC += 2;
					break;

				// 0x8ye
				case 0xe: // shl Vx
					VF = Vx >> 7;
					Vx <<= 1;
					PC += 2;
					break;

				default:
					not_handled(msb, lsb);
			}
		}
		break;

		// 9xy0
		case 0x9: // jneqr Vx, Vy
			if (Vx != Vy) PC += 2;
			PC += 2;
			break;

		// Annn
		case 0xA: // mov I, nnn
			I = nnn;
			PC += 2;
			break;

		// 0xBnnn
		case 0xB: // jmp V0+nnn
			PC = V[0] + nnn;
			break;

		// 0xCxkk
		case 0xC: // rnd Vx, kk
			Vx = uniform_int_distribution<>(0, 255)(rnd) & kk;
			PC += 2;
			break;

		// 0xDxyn
		case 0xD: { // draw Vx, Vy, n
			/*
				Read n bytes from memory, starting at the address stored in I.
				These bytes are then displayed as sprites on screen at coordinates (Vx, Vy), with width = 8 and height = n.
				Sprites are XORed onto the existing screen. If this causes any pixels to be erased, VF is set to 1, otherwise it is set to 0.
				If the sprite is positioned so part of it is outside the coordinates of the display, it wraps around to the opposite side of the screen.
			*/
			byte width = 8,
				height = n,
				*row_pixels = &memory[I];

			int px, py;
			for (int h = 0; h < height; ++h) {
				py = (Vy + h) % 32;

				for (int w = 0; w < width; ++w) {
					px = (Vx + w) % 64;

					if (*row_pixels & (0b10000000 >> w)) { // checks if (8-w)'th bit is set
						byte &pixel = screen[64*py + px];

						VF = (pixel == 1),
						pixel ^= 1;
					}
				}
				++row_pixels;
			}
			redraw = true;
			PC += 2;
		}
		break;

		// 0xEx--
		case 0xE: {
			switch (lsb) {
				// 0xE9E
				case 0x9E: // jkey Vx
					if (key_pressed[Vx]) PC += 2;
					PC += 2;
					break;

				// 0xA1
				case 0xA1: // jnkey Vx
					if (!key_pressed[Vx]) PC += 2;
					PC += 2;
					break;
			}
		}
		break;

		// 0xFx--
		case 0xF: {
			switch (lsb) {
				// 0xFx07
				case 0x07: // getdelay Vx
					Vx = DT;
					PC += 2;
					break;

				// 0xFx0A
				case 0x0A: // waitkey Vx
					awaitingKey = 0x80 | x;
					PC += 2;
					break;

				// 0xFx15
				case 0x15: // setdelay Vx
					DT = Vx;
					PC += 2;
					break;

				// 0xFx18
				case 0x18: // setsound Vx
					ST = Vx;
					PC += 2;
					break;

				// 0xFx1E
				case 0x1E: // mov I, Vx
					I += Vx;
					VF = (I+Vx > 0xff);
					PC += 2;
					break;

				// 0xFx29
				case 0x29: // spritei I, Vx
					I = 5 * Vx;
					PC += 2;
					break;

				// 0xFx33
				case 0x33: // bcd [I], Vx
					tmp = Vx;
					byte a,b,c;
					c = tmp%10, tmp/=10;
					b = tmp%10, tmp/=10;
					a = tmp;

					memory[I] = a, memory[I+1] = b, memory[I+2] = c;
					PC += 2;
					break;

				// 0xFx55
				case 0x55: { // mov [I], V0-VF
					byte *memv = &memory[I];

					for (int p = 0; p <= x; ++p)
						*(memv++) = V[p];

					I += x+1, PC += 2;
				}
				break;

				// 0xFx65
				case 0x65: { // mov V0-VF, [I]
					byte *memv = &memory[I];

					for (int p = 0; p <= x; ++p)
						V[p] = *(memv++);

					I += x+1, PC += 2;
					break;
				}

				default:
					not_handled(msb, lsb);
			}
			break;
		}

		default:
			not_handled(msb, lsb);
	}
	#undef not_handled

    if (DT > 0) --DT;
    if (ST > 0) --ST;

}
