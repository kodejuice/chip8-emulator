/**
 * Chip8 Emulator
 * 
 * (C) Sochima Biereagu, 2019
*/

#include "chip8.cc"
#include "debug/disassembler.cc" // for debugging

#include "SDL2/SDL.h"

string getfilename(string);


// chip8 screen size
const int w = 64, h = 32;

// emulator window size
const int W = w*16, H = h*16;

// RGB buffer
uint32_t pixels[w * h];

// render screen in RGB buffer
void renderTo(uint32_t* pixels, const byte* screen) {
	for (int i = 0; i < w * h; ++i)
		pixels[i] = (0x0033FF66 * screen[i]) | 0xFF111111;
}


static deque<pair<unsigned,bool>> AudioQueue;


#define COMMAND_USAGE\
	return cout << "Command usage:\n ./chip8 <program>\n ./chip8 --decode <program>" << endl, 1;


int main(int argc, char** argv) {
	if (argc < 2)
		COMMAND_USAGE;

	Chip8 cpu = Chip8();

	string arg1 = string(argv[1]);
	if (arg1 == "--decode") {
		// load/decode program
		if (argc < 3) COMMAND_USAGE;
		cpu.load_program(argv[2]);

		cout << cpu.disassemble();
		return 0;
	}
	else {
		// load program
		if (!cpu.load_program(arg1)) exit(1);
	}

	printf("\nPress P to pause emulation.\n");


	/////////
	// GUI //
	/////////

	string filename = getfilename(arg1);

	// create window
	string title = "Chip8 Emulator - " + filename;
	SDL_Window* window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, W, H, SDL_WINDOW_RESIZABLE);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0); SDL_RenderSetLogicalSize(renderer, W, H);
	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);

	// mapping of SDL keyboard symbols to chip8 keypad codes
	unordered_map<int,int> keymap{
		{SDLK_1, 0x1}, {SDLK_2, 0x2}, {SDLK_3, 0x3}, {SDLK_4, 0xC},
		{SDLK_q, 0x4}, {SDLK_w, 0x5}, {SDLK_e, 0x6}, {SDLK_r, 0xD},
		{SDLK_a, 0x7}, {SDLK_s, 0x8}, {SDLK_d, 0x9}, {SDLK_f, 0xE},
		{SDLK_z, 0xA}, {SDLK_x, 0x0}, {SDLK_c, 0xB}, {SDLK_v, 0xF},
		{SDLK_5, 0x5}, {SDLK_6, 0x6}, {SDLK_7, 0x7}, {SDLK_p, -2},
		{SDLK_8, 0x8}, {SDLK_9, 0x9}, {SDLK_0, 0x0}, {SDLK_ESCAPE,-1}
	};

	// Init. SDL audio
	SDL_AudioSpec spec, obtained;
	spec.freq     = 44100;
	spec.format   = AUDIO_S16SYS;
	spec.channels = 2;
	spec.samples  = spec.freq/20;
	spec.callback = [](void*, Uint8* stream, int len) {
		short* target = (short*)stream;
		while(len > 0 && !AudioQueue.empty()) {
			auto& data = AudioQueue.front();
			for(; len && data.first; target += 2, len -= 4, --data.first)
				target[0] = target[1] = data.second*300*((len&128)-64);
			if(!data.first) AudioQueue.pop_front();
		}
	};
	SDL_OpenAudio(&spec, &obtained);
	SDL_PauseAudio(0);


	int msg = 0;
	int frames_done = 0;
	bool running = true,
		 paused = false;


	auto start = chrono::system_clock::now();
	while (running) {

		/////////////////////////
		// Execute Instruction //
		/////////////////////////
		if (!cpu.awaitingKey && !paused)
			// if not waiting for input nor paused
			cpu.emulate_op();

		if (paused && !msg) {
			printf("\nPaused\n");
			msg = 1;
		}

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

					if (i == keymap.end()) break;
					if (i->second == -1) { running = 0; break; }
					if (i->second == -2 && ev.type==SDL_KEYDOWN) {
						paused=!paused, msg=0;
						if (!paused) printf("Resumed\n");
						break;
					}

					cpu.key_pressed[i->second] = (ev.type == SDL_KEYDOWN);

					if(ev.type==SDL_KEYDOWN && cpu.awaitingKey) {
						cpu.V[cpu.awaitingKey & 0x7f] = i->second;
						cpu.awaitingKey = 0;
					}
			}


		/////////////
		// Render  //
		/////////////
		auto cur = chrono::system_clock::now();

		chrono::duration<double> elapsed_seconds = cur-start;
		int frames = int(elapsed_seconds.count() * 60) - frames_done;

		if (frames > 0 && !paused) {
			frames_done += frames;

            // Update the timer registers
            int st = min<int>(frames, cpu.ST); cpu.ST -= st;
            int dt = min<int>(frames, cpu.DT); cpu.DT -= dt;

            //////////////////
            // Render audio //
            SDL_LockAudio();
             AudioQueue.emplace_back( obtained.freq*st/60,  true );
             AudioQueue.emplace_back( obtained.freq*(frames-st)/60, false );
            SDL_UnlockAudio();

			/////////////////////
			// Render graphics //
			if (cpu.redraw) {
				renderTo(pixels, cpu.screen);
				SDL_UpdateTexture(texture, nullptr, pixels, 4*w);
				SDL_RenderClear(renderer);
				SDL_RenderCopy(renderer, texture, nullptr, nullptr);
				SDL_RenderPresent(renderer);

				cpu.redraw = false;
			}
		}

		if (!paused)
			this_thread::sleep_for(chrono::microseconds(1300));
	}

	return 0;
}


/**
 * get filename from filepath
 */
string getfilename(const string filepath) {
	string filename = filepath;

	// Remove directory if present.
	// Do this before extension removal incase directory has a period character.
	const size_t last_slash_idx = filename.find_last_of("\\/");
	if (std::string::npos != last_slash_idx)
		filename.erase(0, last_slash_idx + 1);

	// Remove extension if present.
	const size_t period_idx = filename.rfind('.');
	if (std::string::npos != period_idx)
	    filename.erase(period_idx);

	return filename;
}
