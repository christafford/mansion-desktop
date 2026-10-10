#!/bin/bash
# Create .import config files for all glTF assets

ASSETS_DIR="/home/deck/code/elsewhere/world/assets"

for gltf in "$ASSETS_DIR"/*.gltf; do
    if [ -f "$gltf" ]; then
        filename=$(basename "$gltf")
        md5=$(md5sum "$gltf" | cut -d' ' -f1)
        scn_file="${filename}-${md5}.scn"
        
        cat > "$ASSETS_DIR/${filename}.import" << INNER
[remap]
type="PackedScene"
loader="scene"
cache="imported/${scn_file}"
path="res://assets/${filename}"
INNER
    fi
done
echo "Import configs created"
