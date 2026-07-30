#!/bin/bash
qemu-system-x86_64 -display cocoa,zoom-to-fit=on -cdrom minecraftos.iso -audiodev coreaudio,id=snd0 -machine pcspk-audiodev=snd0
