#include "chip8.cc"
#include "debug/disassembler.cc" // for debugging


int main(int argc, char** argv) {
	srand (time(NULL));

	if (argc != 2) {
		cout << "Command usage:\n ./chip8 [ROM file]" << endl;
		exit(1);
	}

	Chip8 chip8 = Chip8();
	if (!chip8.loadROM(argv[1])) exit(2);

	// cout << chip8.disassemble();

	// printf("%d\n", chip8.screen[100]);



	return 0;
}
