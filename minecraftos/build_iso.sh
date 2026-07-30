#!/bin/bash

# Ensure myos.bin exists
if [ ! -f myos.bin ]; then
    echo "myos.bin not found! Please run your build script first (e.g., ./run.sh without the qemu line)."
    exit 1
fi

echo "Creating ISO directory structure..."
mkdir -p isodir/boot/grub

echo "Copying kernel..."
cp myos.bin isodir/boot/myos.bin

echo "Creating GRUB configuration..."
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "Minecraft OS" {
    multiboot /boot/myos.bin
}
EOF

echo "Building ISO..."
if command -v grub-mkrescue &> /dev/null; then
    grub-mkrescue -o minecraftos.iso isodir
elif command -v grub2-mkrescue &> /dev/null; then
    grub2-mkrescue -o minecraftos.iso isodir
elif command -v i686-elf-grub-mkrescue &> /dev/null; then
    i686-elf-grub-mkrescue -o minecraftos.iso isodir
elif command -v x86_64-elf-grub-mkrescue &> /dev/null; then
    x86_64-elf-grub-mkrescue -o minecraftos.iso isodir
else
    echo "Error: grub-mkrescue or grub2-mkrescue is not installed."
    echo "On macOS, install it with: brew install i686-elf-grub xorriso mtools"
    exit 1
fi

echo "Done! You can run it with: qemu-system-x86_64 -cdrom minecraftos.iso"
