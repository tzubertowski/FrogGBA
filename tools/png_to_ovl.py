#!/usr/bin/env python3
"""
PNG to OVL converter for FrogGBA overlay system
Converts PNG images to .ovl format (RGB5551)

Usage: python png_to_ovl.py input.png output.ovl
"""

import sys
import struct
from PIL import Image

def convert_png_to_ovl(png_path, ovl_path):
    """
    Convert a PNG file to OVL format (RGB5551)
    
    Format: 16-bit per pixel
    - Bit 15: Alpha (1 = visible, 0 = transparent)
    - Bits 14-10: Blue (5 bits)
    - Bits 9-5: Green (5 bits)  
    - Bits 4-0: Red (5 bits)
    """
    
    # Open PNG
    img = Image.open(png_path)
    
    # Convert to RGBA if needed
    if img.mode != 'RGBA':
        img = img.convert('RGBA')
    
    # Resize to GBA resolution if needed
    if img.size != (240, 160):
        print(f"Warning: Resizing image from {img.size} to (240, 160)")
        img = img.resize((240, 160), Image.LANCZOS)
    
    # Create output buffer
    ovl_data = bytearray()
    
    # Process each pixel
    for y in range(160):
        for x in range(240):
            r, g, b, a = img.getpixel((x, y))
            
            # Convert 8-bit to 5-bit color components
            r5 = (r >> 3) & 0x1F
            g5 = (g >> 3) & 0x1F
            b5 = (b >> 3) & 0x1F
            
            # Set alpha bit (bit 15) if pixel is not fully transparent
            # You can adjust the threshold (128) for transparency sensitivity
            alpha_bit = 1 if a > 128 else 0
            
            # Pack into RGB5551 format
            # Format: ABBBBBGGGGGRRRRR (A=alpha, B=blue, G=green, R=red)
            pixel = (alpha_bit << 15) | (b5 << 10) | (g5 << 5) | r5
            
            # Write as little-endian 16-bit value
            ovl_data.extend(struct.pack('<H', pixel))
    
    # Write to file
    with open(ovl_path, 'wb') as f:
        f.write(ovl_data)
    
    print(f"Successfully converted {png_path} to {ovl_path}")
    print(f"Output size: {len(ovl_data)} bytes ({240*160*2} expected)")

def main():
    if len(sys.argv) != 3:
        print("Usage: python png_to_ovl.py input.png output.ovl")
        print("\nExample: python png_to_ovl.py gameboy_overlay.png gameboy_overlay.ovl")
        sys.exit(1)
    
    png_path = sys.argv[1]
    ovl_path = sys.argv[2]
    
    try:
        convert_png_to_ovl(png_path, ovl_path)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()