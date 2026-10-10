#!/bin/bash
# Recreate .import config files to match existing .scn files

SCN_DIR="/home/deck/code/elsewhere/world/.godot/imported"

for scn in "$SCN_DIR"/*.scn; do
    if [ -f "$scn" ]; then
        scn_name=$(basename "$scn")
        # Extract gltf base name and hash from .scn filename
        # Format: name-md5.scn
        gltf_base=$(echo "$scn_name" | sed 's/\.scn$//')
        
        cat > "/home/deck/code/elsewhere/world/assets/${gltf_base}.import" << INNER
[remap]
type="PackedScene"
loader="scene"
cache="imported/${scn_name}"
path="res://assets/${gltf_base%.gltf}.gltf"
INNER
    fi
done
echo "Import configs recreated"
