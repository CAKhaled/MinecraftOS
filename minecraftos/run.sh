#!/bin/bash
python3 build_config.py
python3 build_textures.py
nasm -f elf32 CamMouseSensitivity.asm -o CamMouseSensitivity_asm.o
nasm -f elf32 CameraMove.asm -o cameramove_asm.o
nasm -f elf32 musicbackground.asm -o musicbackground.o
i686-elf-gcc -c CamMouseSensitivity.c -o CamMouseSensitivity.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -c CameraMove.c -o CameraMove.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -c kernel.c -o kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -c textured_cube.c -o textured_cube.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -c MapLoader.c -o MapLoader.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -c BuilderBreaker.c -o BuilderBreaker.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -c splash.c -o splash.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
nasm -f elf32 canvas_ui.asm -o canvas_ui_asm.o
i686-elf-gcc -c canvas_ui.c -o canvas_ui.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -T linker.ld -o myos.bin -ffreestanding -O2 -nostdlib boot.o kernel.o textured_cube.o cuberenderer.o CamMouseSensitivity.o CamMouseSensitivity_asm.o CameraMove.o cameramove_asm.o musicbackground.o MapLoader.o BuilderBreaker.o splash.o canvas_ui.o canvas_ui_asm.o -lgcc
qemu-system-x86_64 -display cocoa,zoom-to-fit=on -full-screen -kernel myos.bin -audiodev coreaudio,id=snd0 -machine pcspk-audiodev=snd0
