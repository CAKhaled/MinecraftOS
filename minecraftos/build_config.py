#!/usr/bin/env python3
"""
build_config.py
Reads config.txt and generates os_config.h with all settings as #defines.
Run this before compiling the kernel so changes to config.txt take effect.
"""

CONFIG_FILE  = "config.txt"
OUTPUT_FILE  = "os_config.h"

# Default values if a key is missing from config.txt
DEFAULTS = {
    "angle_x":            0,
    "angle_y":            0,
    "angle_z":            0,
    "BLOCK_MAP_Y_AXIS":  -160,
    "CAM_SENSITIVITY":    1,
    "JUMP_HEIGHT":        150,
    "CAMERA_FOV":         256,   # 256 ≈ 90°, 128 = narrow, 512 = wide
}

# Maps config.txt key  →  C #define name
KEY_MAP = {
    "angle_x":           "STATIC_ANGLE_X",
    "angle_y":           "STATIC_ANGLE_Y",
    "angle_z":           "STATIC_ANGLE_Z",
    "BLOCK_MAP_Y_AXIS":  "STATIC_BLOCK_MAP_Y_AXIS",
    "CAM_SENSITIVITY":   "STATIC_CAM_SENSITIVITY",
    "JUMP_HEIGHT":       "STATIC_JUMP_HEIGHT",
    "CAMERA_FOV":        "STATIC_CAMERA_FOV",
}

def parse_config(path):
    values = dict(DEFAULTS)
    try:
        with open(path, "r") as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                if "=" in line:
                    key, _, val = line.partition("=")
                    key = key.strip()
                    val = val.strip()
                    if key in values:
                        try:
                            values[key] = int(val)
                        except ValueError:
                            pass  # keep default for non-integer values
    except OSError:
        print(f"  [warn] {path} not found – using defaults")
    return values

def write_header(values, path):
    with open(path, "w", encoding="ascii") as f:
        print("#ifndef OS_CONFIG_H", file=f)
        print("#define OS_CONFIG_H", file=f)
        print("", file=f)
        for cfg_key, c_name in KEY_MAP.items():
            val = values.get(cfg_key, DEFAULTS[cfg_key])
            print(f"#define {c_name} {val}", file=f)
        print("", file=f)
        print("#endif", file=f)

def main():
    values = parse_config(CONFIG_FILE)
    write_header(values, OUTPUT_FILE)
    print(f"Generated {OUTPUT_FILE}:")
    for cfg_key, c_name in KEY_MAP.items():
        print(f"  {c_name} = {values[cfg_key]}")

if __name__ == "__main__":
    main()
