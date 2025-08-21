#!/bin/bash
# Convert all PNG files in current directory to OVL format

echo "Converting all PNG files to OVL format..."

for png_file in *.png; do
    if [ -f "$png_file" ]; then
        # Get filename without extension
        base_name="${png_file%.png}"
        ovl_file="${base_name}.ovl"
        
        echo "Converting $png_file -> $ovl_file"
        python3 png_to_ovl.py "$png_file" "$ovl_file"
    fi
done

echo "Conversion complete!"