# Minecraft OS

A custom operating system built completely from scratch that boots directly into a Minecraft-inspired voxel world.

## ✨ Features

- Custom 32-bit operating system
- Written in C and x86 Assembly
- Boots without any existing operating system
- Software-rendered 3D voxel engine
- Minecraft-inspired world generation
- First-person camera
- Keyboard and mouse controls
- Chunk-based world rendering
- Texture mapping
- Basic game UI (Hotbar, Crosshair, Block Selection)

## 🛠 Technologies

- C
- x86 Assembly
- NASM
- GCC (i686-elf)
- Custom Kernel
- Software Rendering (No OpenGL/DirectX)

## 🚀 Getting Started

### Build

```bash
build.bat
```

### Run

```bash
qemu-system-i386 -cdrom minecraftos.iso -boot d -m 512
```

## 📸 Preview

> *(Add screenshots or gameplay GIFs here.)*

## 🎯 Goal

The goal of this project is to learn low-level programming by building a complete operating system capable of running a Minecraft-inspired voxel engine from scratch.

## 📄 License

Khaled AlZahrani - 2026
