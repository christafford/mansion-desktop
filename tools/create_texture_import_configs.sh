#!/bin/bash
# Create .import config files for texture assets

SCN_DIR="/home/deck/code/mansion-desktop/world/.godot/imported"

# Process all .md5 files to find source textures
for md5_file in "$SCN_DIR"/*.md5; do
    if [ -f "$md5_file" ]; then
        # Check if this is a texture (not a .gltf file)
        md5_content=$(cat "$md5_file")
        source_md5=$(echo "$md5_content" | grep "source_md5" | cut -d'"' -f2)
        
        # Get the filename from md5 file (without .md5)
        basename=$(basename "$md5_file" .md5)
        
        # Skip .gltf files - they're handled separately
        if [[ "$basename" == *.gltf-* ]]; then
            continue
        fi
        
        # This is a texture, create .import file
        # Godot stores texture imports in the same directory with .import extension
        # Format: source_file-md5.import
        import_file="${SCN_DIR}/${basename}.import"
        
        cat > "$import_file" << INNER
[remap]
type="CompressedTexture3D"
loader="texture"
cache="imported/${basename}.ctex"
path="res://assets/${basename%.gltf-*}.gltf"
INNER
        echo "Created: $(basename "$import_file")"
    fi
done
