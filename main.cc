/**
 * Chip8 Emulator
 * 
 * (C) Sochima Biereagu, 2019
*/

#include <thread>
#include <chrono>
#include <unordered_map>

#include "chip8.cc"
#include "debug/disassembler.cc" // for debugging

#include "SDL2/SDL.h"


// TODO: Implement GUI w/ SDL
// TODO: Beep Sound


// chip8 screen size
const int w = 64, h = 32;

// emulator window size
const int W = w*16, H = h*16;

// RGB buffer
uint32_t pixels[w * h];

// render screen in RGB buffer
void renderTo(uint32_t* pixels, const byte* screen) {
	for (int i = 0; i < w * h; ++i)
		pixels[i] = (0x00FFFFFF * screen[i]) | 0xFF111111;
}


int main(int argc, char** argv) {
	if (argc != 2)
		return cout << "Command usage:\n ./chip8 <program>" << endl, 1;

	Chip8 cpu = Chip8();
	if (!cpu.load_program(argv[1])) exit(1);
	// cout << cpu.disassemble();

	/////////
	// GUI //
	/////////

	// create window
	string title = string(argv[1]);
	SDL_Window* window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, W, H, SDL_WINDOW_RESIZABLE);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0); SDL_RenderSetLogicalSize(renderer, W, H);
	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);

	// mapping of SDL keyboard symbols to chip8 keypad codes
	std::unordered_map<int,int> keymap{
		{SDLK_1, 0x1}, {SDLK_2, 0x2}, {SDLK_3, 0x3}, {SDLK_4, 0xC},
		{SDLK_q, 0x4}, {SDLK_w, 0x5}, {SDLK_e, 0x6}, {SDLK_r, 0xD},
		{SDLK_a, 0x7}, {SDLK_s, 0x8}, {SDLK_d, 0x9}, {SDLK_f, 0xE},
		{SDLK_z, 0xA}, {SDLK_x, 0x0}, {SDLK_c, 0xB}, {SDLK_v, 0xF},
		{SDLK_5, 0x5}, {SDLK_6, 0x6}, {SDLK_7, 0x7},
		{SDLK_8, 0x8}, {SDLK_9, 0x9}, {SDLK_0, 0x0}, {SDLK_ESCAPE,-1}
	};


	bool running = true;
	while (running) {
		// execute while not waiting for input
		if (!cpu.awaitingKey)
			cpu.emulate_op();

		////////////////////
		// Process events //
		////////////////////
		for(SDL_Event ev; SDL_PollEvent(&ev); )
			switch(ev.type)
			{
				case SDL_QUIT: running = 0; break;
				case SDL_KEYDOWN:
				case SDL_KEYUP:
					auto i = keymap.find(ev.key.keysym.sym);

					if(i == keymap.end()) break;
					if(i->second == -1) { running = 0; break; }

					cpu.key_pressed[i->second] = (ev.type == SDL_KEYDOWN);

					if(ev.type==SDL_KEYDOWN && cpu.awaitingKey) {
						cpu.V[cpu.awaitingKey & 0x7f] = i->second;
						cpu.awaitingKey = 0;
					}
			}


		/////////////////////
		// Render graphics //
		///////////////./////

		if (cpu.redraw) {
			renderTo(pixels, cpu.screen);
			SDL_UpdateTexture(texture, nullptr, pixels, 4*w);
			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, texture, nullptr, nullptr);
			SDL_RenderPresent(renderer);

			cpu.redraw = false;
		}

        std::this_thread::sleep_for(std::chrono::microseconds(1300));
	}

	return 0;
}
