# Minecraft OS

A custom operating system built completely from scratch that boots directly into a Minecraft-inspired voxel world.

## ✨ Features

- Custom 32-bit operating system
- Written in C and x86 Assembly
- Boots without any existing operating system
- Software-rendered 3D voxel engine
- Minecraft-inspired voxel world
- First-person camera
- Keyboard and mouse controls
- Chunk-based rendering
- Texture mapping
- Basic game UI (Hotbar, Crosshair, Block Selection)

## 🛠 Technologies

- C
- x86 Assembly
- NASM
- GCC (i686-elf)
- Custom Kernel
- Software Rendering

## 🚀 Build

Build the project using:

```bash
build.bat
```

## ▶️ Run

After building, the generated ISO will be located at:

```text
extracted_iso/minecraftos.iso
```

Run it using QEMU:

```bash
qemu-system-i386 -cdrom extracted_iso/minecraftos.iso -boot d -m 512
```

## 📝 Notes

> **Important:** This project cannot be built or modified directly on Windows.
>
> To edit or compile the operating system, use **Linux** or **Windows Subsystem for Linux (WSL)** with the required cross-compilation toolchain installed.

## 📸 Preview

Add screenshots or gameplay GIFs here.

## 🎯 Project Goal

The goal of this project is to learn low-level programming by building a complete operating system capable of running a Minecraft-inspired voxel engine from scratch.

## 📄 License

Khaled AlZahrani - 2026
