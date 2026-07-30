import os
try:
    from PIL import Image
except ImportError:
    print("Please install Pillow: pip3 install pillow")
    exit(1)

# Generate a proper 256-color palette (like web-safe or standard VGA 256)
# 6x6x6 color cube = 216 colors + 40 grayscale
vga_palette = []
for r in range(6):
    for g in range(6):
        for b in range(6):
            vga_palette.extend([r * 51, g * 51, b * 51])
for i in range(40):
    vga_palette.extend([i * 6, i * 6, i * 6])

def generate_dummy_raw(pattern_type):
    data = []
    for y in range(64):
        for x in range(64):
            if pattern_type == 'checker':
                color = 215 if ((x // 8) + (y // 8)) % 2 == 0 else 0
            else:
                color = 125 if (x // 8) % 2 == 0 else 30
            data.append(color)
    return data

def process_image(filepath, default_pattern, size=(64, 64)):
    if not os.path.exists(filepath):
        print(f"Warning: {filepath} not found. Generating dummy texture.")
        return [0] * (size[0] * size[1])
    
    if filepath.endswith('.raw'):
        print(f"Loading 8-bit RAW file: {filepath}")
        with open(filepath, 'rb') as f:
            data = f.read(size[0]*size[1])
            if len(data) < size[0]*size[1]:
                data += b'\x00' * (size[0]*size[1] - len(data))
            return list(data[:size[0]*size[1]])

    try:
        img = Image.open(filepath).convert('RGB')
        img = img.resize(size)
        
        pal_img = Image.new('P', (1, 1))
        pal_img.putpalette(vga_palette)
        img = img.quantize(palette=pal_img, dither=Image.Dither.NONE)
        
        return list(img.getdata())
    except Exception as e:
        print(f"Error processing {filepath}: {e}")
        return [0] * (size[0] * size[1])

def main():
    config_file = 'config.txt'
    top_bottom_path = 'images/top_bottom.jpg'
    sides_path = 'images/sides.jpg'
    top_bottom_stone_path = 'images/stone.jpg'
    sides_stone_path = 'images/stone.jpg'
    ui_grass_path = 'images/top_bottom.jpg'
    ui_stone_path = 'images/stone.jpg'
    
    # Default rotation angles
    angle_x = 0
    angle_y = 0
    angle_z = 0
    block_map_y_axis = 1
    cam_sensitivity = 1
    jump_height = 150

    if os.path.exists(config_file):
        with open(config_file, 'r') as f:
            for line in f:
                if '=' in line:
                    key, val = line.strip().split('=', 1)
                    if key == 'top_bottom_texture':
                        top_bottom_path = val
                    elif key == 'sides_texture':
                        sides_path = val
                    elif key == 'top_bottom_stone':
                        top_bottom_stone_path = val
                    elif key == 'sides_stone':
                        sides_stone_path = val
                    elif key == 'ui_grass':
                        ui_grass_path = val
                    elif key == 'ui_stone':
                        ui_stone_path = val
                    elif key == 'angle_x':
                        angle_x = int(val)
                    elif key == 'angle_y':
                        angle_y = int(val)
                    elif key == 'angle_z':
                        angle_z = int(val)
                    elif key == 'BLOCK_MAP_Y_AXIS':
                        block_map_y_axis = int(val)
                    elif key == 'CAM_SENSITIVITY':
                        cam_sensitivity = int(val)
                    elif key == 'JUMP_HEIGHT':
                        jump_height = int(val)

    tex_tb = process_image(top_bottom_path, 'checker')
    tex_sides = process_image(sides_path, 'stripes')
    tex_tb_stone = process_image(top_bottom_stone_path, 'checker')
    tex_sides_stone = process_image(sides_stone_path, 'stripes')
    tex_ui_grass = process_image(ui_grass_path, 'checker', size=(12, 12))
    tex_ui_stone = process_image(ui_stone_path, 'stripes', size=(12, 12))

    with open('os_config.h', 'w') as f:
        f.write("#ifndef OS_CONFIG_H\n#define OS_CONFIG_H\n\n")
        f.write(f"#define STATIC_ANGLE_X {angle_x}\n")
        f.write(f"#define STATIC_ANGLE_Y {angle_y}\n")
        f.write(f"#define STATIC_ANGLE_Z {angle_z}\n")
        f.write(f"#define STATIC_BLOCK_MAP_Y_AXIS {block_map_y_axis}\n")
        f.write(f"#define STATIC_CAM_SENSITIVITY {cam_sensitivity}\n")
        f.write(f"#define STATIC_JUMP_HEIGHT {jump_height}\n\n")
        f.write("#endif\n")

    with open('textures.h', 'w') as f:
        f.write("#ifndef TEXTURES_H\n#define TEXTURES_H\n\n")
        f.write("#include <stdint.h>\n\n")
        
        f.write("static const uint8_t vga_palette_data[256 * 3] = {\n")
        f.write(", ".join(str(b) for b in vga_palette))
        f.write("\n};\n\n")

        f.write("static const uint8_t tex_top_bottom[4096] = {\n")
        f.write(", ".join(str(b) for b in tex_tb))
        f.write("\n};\n\n")

        f.write("static const uint8_t tex_sides[4096] = {\n")
        f.write(", ".join(str(b) for b in tex_sides))
        f.write("\n};\n\n")

        f.write("static const uint8_t tex_top_bottom_stone[4096] = {\n")
        f.write(", ".join(str(b) for b in tex_tb_stone))
        f.write("\n};\n\n")

        f.write("static const uint8_t tex_sides_stone[4096] = {\n")
        f.write(", ".join(str(b) for b in tex_sides_stone))
        f.write("\n};\n\n")

        f.write("static const uint8_t tex_ui_grass[144] = {\n")
        f.write(", ".join(str(b) for b in tex_ui_grass))
        f.write("\n};\n\n")

        f.write("static const uint8_t tex_ui_stone[144] = {\n")
        f.write(", ".join(str(b) for b in tex_ui_stone))
        f.write("\n};\n\n")
        
        f.write("#endif\n")
    print("Generated textures.h successfully with full 256 color palette!")

if __name__ == '__main__':
    main()
