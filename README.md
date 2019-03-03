# Chip8 Emulator

Chip-8 is a programming language for 8-bit computers developed in the 1970s. It is an interpreted language that was intended for game development. It originally ran on the COSMAC VIP and Telmac 1800 computers.

![Tank](screenshots/tank.png "Tank")
*Tank*

![Brix](screenshots/brix.png "Brix")
*Brix*

![Pong](screenshots/pong.png "Pong")
*Pong*

## Compiling

### Installing requirements
It requires the [SDL](https://www.libsdl.org) library for rendering.

Debian-based
```
$ sudo apt-get install libsdl2-dev
```

Arch-based
```
$ pacman -S sdl2
```

### Compile

```
$ make
```

*Run*

```
$ ./chip8 <program>
```

The repo comes with a set of programs in the `roms/` directory.
