#!/usr/bin/env python3
"""Stage complete glTF packages from the existing cache without network access.

Import source glTF, never Godot's version-specific .godot/imported files.
The source cache stays ignored by Godot so large unused HDRIs aren't imported.
"""
import json
import hashlib
import shutil
from pathlib import Path


def prepare(root: Path) -> None:
    assets = root / "assets"
    cache = assets / "cache"
    manifest = json.loads((assets / "manifest.json").read_text())
    count = 0
    for asset in manifest["assets"]:
        if asset["type"] != "model":
            continue
        source = cache / (asset["id"] + "_glb.gltf")
        model = json.loads(source.read_text())
        copies = [(source, assets / (asset["id"] + ".gltf"), asset["hash"]["gltf"].removeprefix("sha256:"))]
        for kind in ("buffers", "images"):
            for item in model.get(kind, []):
                uri = item.get("uri", "")
                if not uri or uri.startswith("data:"):
                    continue
                relative = Path(uri)
                if relative.is_absolute() or ".." in relative.parts or ":" in uri:
                    raise ValueError(f"Unsafe asset URI: {uri}")
                hashes = asset["hash"]["buffers" if kind == "buffers" else "textures"]
                copies.append((cache / relative, assets / relative, hashes[relative.name]))
        for src, dst, expected in copies:
            if not src.is_file():
                raise FileNotFoundError(f"Missing model dependency: {src}")
            if hashlib.sha256(src.read_bytes()).hexdigest() != expected:
                raise ValueError(f"SHA-256 mismatch for {src}")
            dst.parent.mkdir(parents=True, exist_ok=True)
            if not dst.exists() or src.read_bytes() != dst.read_bytes():
                shutil.copyfile(src, dst)
        count += 1
    # Architectural maps are already in the source cache; no new downloads.
    for name in ("walnut_veneer_4k_jpg.jpg", "beige_wall_001_4k_jpg.jpg"):
        src, dst = cache / name, assets / "textures" / name
        if not dst.exists() or src.read_bytes() != dst.read_bytes():
            shutil.copyfile(src, dst)
    print(f"Prepared {count} complete model packages from local cache")


if __name__ == "__main__":
    prepare(Path(__file__).resolve().parents[1])
