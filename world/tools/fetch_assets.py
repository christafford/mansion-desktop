#!/usr/bin/env python3
"""
Mansion Desktop — Asset Fetcher and Verifier (P21-T03)

Fetches Poly Haven assets from the curated manifest, verifies MD5 hashes
from the API response, and manages a local cache with atomic downloads
and finite retries.

Usage:
    python3 fetch_assets.py [--all] [--force] [--hashes] [--cache-dir DIR]
                            [--download-dir DIR] [--dry-run] [--quiet]

Options:
    --all          Fetch all assets including alternates (default: primary only)
    --force        Re-download even if cached and hash matches
    --hashes       Print MD5 hashes for all cached assets and exit
    --cache-dir    Local cache directory (default: assets/cache)
    --download-dir Where to place fetched assets (default: same as cache)
    --dry-run      Show what would be downloaded without downloading
    --quiet        Suppress progress output

Poly Haven API:
    Asset list:     https://api.polyhaven.com/assets
    Asset files:    https://api.polyhaven.com/files/<id>
    Direct download: https://polyhaven.com/assets/<name>

All assets are CC0 (public domain). See https://polyhaven.com/license
"""

from __future__ import annotations

import argparse
import hashlib
import json
import logging
import os
import sys
import tempfile
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path
from typing import Optional
from urllib.request import Request, urlopen
from urllib.error import HTTPError, URLError
from concurrent.futures import ThreadPoolExecutor, as_completed
import shutil

logger = logging.getLogger("mansion.fetch")

# ─── Manifest ────────────────────────────────────────────────────────

# Curated asset set from P21-T02 manifest.
# Format: (asset_id, file_key, priority, file_type)
# file_key maps to resolution/form
ASSET_LIST: list[tuple[str, str, str]] = [
    # HDRI
    ("poly_haven_studio", "24k_exr", "primary"),
    ("poly_haven_studio", "24k_jpg", "primary"),
    ("anniversary_lounge", "16k_jpg", "primary"),
    ("lebombo", "16k_jpg", "primary"),
    # Textures
    ("walnut_veneer", "4k_jpg", "primary"),
    ("walnut_veneer", "4k_normal_opengl", "primary"),
    ("walnut_veneer", "4k_roughness", "primary"),
    ("beige_wall_001", "4k_jpg", "primary"),
    ("beige_wall_001", "4k_normal_opengl", "primary"),
    ("beige_wall_001", "4k_roughness", "primary"),
    ("white_plaster_02", "2k_jpg", "primary"),
    ("white_plaster_02", "2k_normal_opengl", "primary"),
    ("curly_teddy_natural", "4k_jpg", "primary"),
    ("curly_teddy_natural", "4k_normal_opengl", "primary"),
    ("brown_leather", "4k_jpg", "primary"),
    ("brown_leather", "4k_normal_opengl", "primary"),
    # Models (GLB not provided by API; falls back to GLTF with .bin buffers)
    ("metal_office_desk", "glb", "primary"),
    ("dining_chair_02", "glb", "primary"),
    ("wooden_bookshelf_worn", "glb", "primary"),
    ("desk_lamp_arm_01", "glb", "primary"),
    ("potted_plant_02", "glb", "primary"),
    ("book_encyclopedia_set_01", "glb", "primary"),
]

# Expected MD5 hashes for verification.
# Populated from the first successful download pass (P21-T03).
# Format: (asset_id, file_key) -> md5_hex
EXPECTED_HASHES: dict[tuple[str, str], str] = {
    ("anniversary_lounge", "16k_jpg"): "3df39e24797865650d33278341d77847",
    ("beige_wall_001", "4k_jpg"): "626521ffbdb51ec8bb35b089b22d2e84",
    ("beige_wall_001", "4k_normal_opengl"): "42c3e5549ab9abd76e2fa0351a812455",
    ("beige_wall_001", "4k_roughness"): "8e35f796e1defb23cb471fea3c8efd3f",
    ("book_encyclopedia_set_01", "glb"): "b5c1936b6b9b0883b7e817ff49f03388",
    ("brown_leather", "4k_jpg"): "3fda61feb484aaf391b0b1a7b9f4804f",
    ("brown_leather", "4k_normal_opengl"): "3f624b698224104e9c18f9fed16df3b0",
    ("curly_teddy_natural", "4k_jpg"): "f4b1c26353193eaa9e7f715ab4a3186d",
    ("curly_teddy_natural", "4k_normal_opengl"): "11d1fc73ec056e1cdd7ee29f7eee72ba",
    ("desk_lamp_arm_01", "glb"): "93587e189642dc55cd6567703dc88fd6",
    ("dining_chair_02", "glb"): "a8f25a5452e75fe1246624bb80f2d7f0",
    ("lebombo", "16k_jpg"): "64ac9ae7e4711d96da237d85e929337a",
    ("metal_office_desk", "glb"): "f8b7985eb522e5c3508ccff43c05d922",
    ("poly_haven_studio", "24k_exr"): "007b1bfa236b2f23042a0d5cd422e20e",
    ("poly_haven_studio", "24k_jpg"): "b0318b69cb1619e7a36283df441e087c",
    ("potted_plant_02", "glb"): "9161f84da94065f5b3888347c0f275ce",
    ("walnut_veneer", "4k_jpg"): "87ff40ee00151386f9db42ee7cd33a9f",
    ("walnut_veneer", "4k_normal_opengl"): "96a8129dcdf6ee32bfb4076ca4062c3d",
    ("walnut_veneer", "4k_roughness"): "31debbbee257cd47dbc9d160dfe82776",
    ("white_plaster_02", "2k_jpg"): "35f9c96c16aa0d934da7884b232f6d07",
    ("white_plaster_02", "2k_normal_opengl"): "a9fa643c4a200686ad89598b545b2adc",
    ("wooden_bookshelf_worn", "glb"): "160c74db04f4893185f1c14d1d59eb1a",
}

# Maximum number of retries for a single download attempt.
MAX_RETRIES = 3

# Delay between retries (seconds), with exponential backoff.
RETRY_DELAY_BASE = 2.0

# Poly Haven API base URL for file metadata.
API_BASE = "https://api.polyhaven.com"

# Timeout for API requests (seconds).
API_TIMEOUT = 30


class DownloadStatus(Enum):
    """Status of a single asset download."""
    NOT_FOUND = "not_found"
    UP_TO_DATE = "up_to_date"
    DOWNLOADING = "downloading"
    SUCCESS = "success"
    FAILED = "failed"
    SKIPPED = "skipped"
    HASH_MISMATCH = "hash_mismatch"


@dataclass
class AssetInfo:
    """Metadata about a single asset file to download."""
    asset_id: str
    file_key: str
    priority: str
    download_url: str = ""
    file_size: int = 0
    local_path: Path = field(repr=False, default=Path())
    status: DownloadStatus = DownloadStatus.NOT_FOUND
    hash_expected: str = ""
    hash_actual: str = ""


# ─── Poly Haven API ──────────────────────────────────────────────────


def fetch_asset_files(asset_id: str) -> Optional[dict]:
    """
    Fetch the list of available files for an asset from the Poly Haven API.

    Endpoint: GET https://api.polyhaven.com/files/<asset_id>
    Returns the JSON response as a dict, or None on failure.
    """
    url = f"{API_BASE}/files/{asset_id}"
    req = Request(url)
    req.add_header("Accept", "application/json")
    req.add_header(
        "User-Agent",
        "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36",
    )

    try:
        with urlopen(req, timeout=API_TIMEOUT) as resp:
            if resp.status != 200:
                logger.warning("API returned %d for %s", resp.status, asset_id)
                return None
            return json.loads(resp.read().decode("utf-8"))
    except (HTTPError, URLError) as e:
        logger.warning("API error for %s: %s", asset_id, e)
        return None


# ─── File Key Resolver ───────────────────────────────────────────────

# HDRI resolution mapping: user key -> API resolution key
# The API stores 16k as "16k+" but 24k as "24k".
HDR_RESOLUTION_MAP = {
    "1k": "1k",
    "2k": "2k",
    "4k": "4k",
    "8k": "8k",
    "16k": "16k+",
    "24k": "24k",
}

# Texture resolution ordering for fallback (higher priority first)
TEXTURE_RESOLUTIONS = ["16k", "8k", "4k", "2k", "1k"]

# Map texture format key suffix to API format key
TEXTURE_FORMAT_MAP = {
    "jpg": "jpg",
    "png": "png",
    "exr": "exr",
}

# Map texture map type key to API top-level key
TEXTURE_MAP_TYPE_MAP = {
    "diffuse": "Diffuse",
    "color": "Diffuse",
    "normal": "nor_gl",
    "normal_opengl": "nor_gl",
    "normal_dx": "nor_dx",
    "rough": "Rough",
    "roughness": "Rough",
    "ao": "AO",
    "ambient_occlusion": "AO",
}


def resolve_hdpi_key(file_key: str, api_data: dict) -> Optional[tuple[str, str]]:
    """Resolve an HDRI file key to (resolution, format) API path.

    Examples:
        "24k_exr" -> ("16k+", "exr")
        "16k_jpg" -> ("8k", "jpg")   (no 16k jpg, falls back to 8k)
        "24k_jpg" -> ("16k+", "jpg") (tonemapped)
    """
    # Tonemapped JPG is always available at top level
    if file_key.endswith("_jpg"):
        if "tonemapped" in api_data:
            return ("tonemapped", "jpg")
        # Fallback: try 8k jpg
        return ("8k", "jpg")

    # Extract resolution and format
    parts = file_key.rsplit("_", 1)
    if len(parts) != 2:
        return None
    resolution, fmt = parts

    api_res = HDR_RESOLUTION_MAP.get(resolution)
    if not api_res:
        return None

    # Check for tonemapped jpg fallback
    if fmt == "jpg" and "tonemapped" in api_data:
        return ("tonemapped", "jpg")

    hdri = api_data.get("hdri", {})
    if api_res not in hdri:
        return None
    res_data = hdri[api_res]
    if fmt in res_data:
        return (api_res, fmt)

    return None


def resolve_texture_key(file_key: str, api_data: dict) -> Optional[tuple[str, str, str]]:
    """Resolve a texture file key to (map_type, resolution, format) API path.

    Examples:
        "4k_jpg"          -> ("Diffuse", "4k", "jpg")
        "4k_normal_opengl" -> ("nor_gl", "4k", "jpg")
        "4k_roughness"     -> ("Rough", "4k", "jpg")
        "2k_jpg"           -> ("Diffuse", "2k", "jpg")
    """
    # Parse the key: <resolution>_<map_type>[_format]
    # e.g. "4k_jpg", "4k_normal_opengl", "4k_roughness"
    parts = file_key.split("_")
    if len(parts) < 2:
        return None

    resolution = parts[0]  # 4k, 2k, etc.
    rest = "_".join(parts[1:])  # "jpg", "normal_opengl", "roughness"

    # Determine format
    if rest in TEXTURE_FORMAT_MAP:
        fmt = rest
        map_type = "diffuse"
    elif rest.endswith("_" + resolution):
        # Unlikely but handle gracefully
        return None
    else:
        map_type = rest
        fmt = "jpg"  # default for normal maps, roughness, etc.

    api_map = TEXTURE_MAP_TYPE_MAP.get(map_type)
    if not api_map:
        return None

    if api_map not in api_data:
        return None

    # Try specified resolution first, then fall back to higher ones
    for res in [resolution] + TEXTURE_RESOLUTIONS:
        if res not in api_data[api_map]:
            continue
        res_data = api_data[api_map][res]
        if fmt in res_data:
            return (api_map, res, fmt)

    return None


def resolve_model_key(file_key: str, api_data: dict) -> Optional[str]:
    """Resolve a model file key to an API path string.

    Returns a dotted path string like "gltf.2k.gltf".
    Note: Poly Haven API doesn't provide direct GLB files.
    GLB format is embedded in gltf responses with .bin files.
    """
    # Direct keys
    if file_key in api_data:
        entry = api_data[file_key]
        if isinstance(entry, dict) and "url" in entry:
            return file_key

    # "glb" -> fall back to gltf.2k.gltf
    if file_key == "glb":
        if "gltf" in api_data:
            return "gltf.2k.gltf"

    # "blend" -> blend.2k.blend (try 2k first, then 1k)
    if file_key == "blend":
        if "blend" in api_data:
            return "blend.2k.blend" if "2k" in api_data["blend"] else "blend.1k.blend"

    return None


def resolve_file_url(api_data: dict, file_key: str) -> Optional[str]:
    """Resolve a file key to a download URL from API data.

    Tries model → texture → HDRI resolvers in order.
    The texture resolver is tried before HDRI because keys like
    '4k_jpg' (texture diffuse) are ambiguous with HDRI '24k_jpg'.
    Returns the URL string, or None if not found.
    """
    # 1. Model formats are unambiguous
    if file_key in ("glb", "blend", "fbx", "obj"):
        path = resolve_model_key(file_key, api_data)
        if path:
            return _get_nested(api_data, path)

    # 2. Try texture resolver first (more specific patterns)
    tex_parts = resolve_texture_key(file_key, api_data)
    if tex_parts:
        map_type, res, fmt = tex_parts
        return _get_nested(api_data, f"{map_type}.{res}.{fmt}.url")

    # 3. Try HDRI resolver (tonemapped JPG, EXR)
    hdri_parts = resolve_hdpi_key(file_key, api_data)
    if hdri_parts:
        res, fmt = hdri_parts
        return _get_nested(api_data, f"hdri.{res}.{fmt}.url") or \
               _get_nested(api_data, f"tonemapped.url")

    return None


def resolve_include_files(api_data: dict, file_key: str) -> Optional[dict]:
    """Resolve a file key to its included files from API data.
    
    For glTF models, returns the 'include' dict with .bin and texture files.
    For other formats, returns None.
    """
    # Only glTF models have includes
    if file_key != "glb":
        return None
        
    path = resolve_model_key(file_key, api_data)
    if not path:
        return None
        
    # Navigate the nested dict to get the glTF entry
    current = api_data
    for key in path.split("."):
        if isinstance(current, dict) and key in current:
            current = current[key]
        else:
            return None
    
    # The 'include' is inside this entry
    if isinstance(current, dict):
        return current.get("include")
    return None


def _get_nested(data: dict, dotted_path: str) -> Optional[str]:
    """Get a value from a nested dict using a dotted path like 'a.b.c'.
    Handles special keys like '16k+' and 'tonemapped'.
    """
    current = data
    for key in dotted_path.split("."):
        if isinstance(current, dict):
            current = current.get(key)
        else:
            return None
    if isinstance(current, dict):
        return current.get("url")
    if isinstance(current, str):
        return current
    return None


# ─── MD5 Verification ────────────────────────────────────────────────


def compute_md5(filepath: Path) -> str:
    """Compute MD5 hash of a file."""
    h = hashlib.md5()
    with open(filepath, "rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def verify_md5(filepath: Path, expected: str) -> bool:
    """Verify file MD5 hash matches expected value."""
    if not expected:
        return True  # No expected hash to verify against
    actual = compute_md5(filepath)
    return actual == expected


# ─── Atomic Download ─────────────────────────────────────────────────


def atomic_download(url: str, dest: Path, dry_run: bool = False,
                    retries: int = MAX_RETRIES) -> bool:
    """
    Download a file atomically (write to temp file, then rename).

    Returns True on success.
    """
    if dry_run:
        return True

    dest.parent.mkdir(parents=True, exist_ok=True)

    for attempt in range(1, retries + 1):
        try:
            fd, tmp_path = tempfile.mkstemp(
                dir=str(dest.parent),
                prefix=f".mansion_{dest.name}.",
            )
            os.close(fd)

            req = Request(url)
            req.add_header(
                "User-Agent",
                "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
                "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36",
            )

            with urlopen(req, timeout=API_TIMEOUT) as resp:
                total = resp.headers.get("Content-Length")
                if total:
                    total = int(total)

                downloaded = 0
                with open(tmp_path, "wb") as f:
                    while True:
                        chunk = resp.read(65536)
                        if not chunk:
                            break
                        f.write(chunk)
                        downloaded += len(chunk)

            dest.parent.mkdir(parents=True, exist_ok=True)
            os.replace(tmp_path, str(dest))
            return True

        except (HTTPError, URLError, OSError) as e:
            try:
                os.unlink(tmp_path)
            except OSError:
                pass

            if attempt < retries:
                delay = RETRY_DELAY_BASE ** attempt
                logger.warning(
                    "Download failed (attempt %d/%d): %s — retrying in %.0fs",
                    attempt, retries, e, delay,
                )
                import time
                time.sleep(delay)
            else:
                logger.error("Download failed after %d retries: %s", retries, e)
                return False

    return False


def download_included_files(api_data: dict, base_path: Path, file_key: str,
                            dry_run: bool = False, quiet: bool = False) -> list[tuple[Path, str, DownloadStatus]]:
    """
    Download all files referenced by a glTF asset's 'include' section.
    
    Returns list of (path, md5_hash, status) tuples.
    """
    results = []
    include = resolve_include_files(api_data, file_key)
    if not include:
        return results
    
    for rel_path, file_info in include.items():
        url = file_info.get("url")
        md5_hash = file_info.get("md5", "")
        
        if not url:
            continue
        
        dest_path = base_path / rel_path
        
        # Check if already cached and valid
        if dest_path.exists():
            if md5_hash and verify_md5(dest_path, md5_hash):
                if not quiet:
                    print(f"  ✓ {rel_path} — cached", flush=True)
                results.append((dest_path, md5_hash, DownloadStatus.UP_TO_DATE))
                continue
        
        # Download the file
        if not dry_run:
            if not quiet:
                print(f"  ↓ {rel_path} ...", end=" ", flush=True)
            
            success = atomic_download(url, dest_path)
            
            if success:
                if md5_hash and verify_md5(dest_path, md5_hash):
                    if not quiet:
                        print("✓", flush=True)
                    results.append((dest_path, md5_hash, DownloadStatus.SUCCESS))
                else:
                    if not quiet:
                        print("✗ (hash mismatch)", flush=True)
                    dest_path.unlink(missing_ok=True)
                    results.append((dest_path, md5_hash, DownloadStatus.HASH_MISMATCH))
            else:
                if not quiet:
                    print("✗", flush=True)
                results.append((dest_path, md5_hash, DownloadStatus.FAILED))
        else:
            if not quiet:
                print(f"  → {rel_path} ({url})", flush=True)
            results.append((dest_path, md5_hash, DownloadStatus.DOWNLOADING))
    
    return results


# ─── Main Fetcher ────────────────────────────────────────────────────


def build_asset_list(all_assets: bool = False) -> list[AssetInfo]:
    """Build the list of assets to fetch from the manifest."""
    assets = []
    for asset_id, file_key, priority in ASSET_LIST:
        if not all_assets and priority != "primary":
            continue
        assets.append(AssetInfo(
            asset_id=asset_id,
            file_key=file_key,
            priority=priority,
        ))
    return assets


def resolve_asset_cache(asset_id: str, file_key: str) -> Path:
    """
    Compute the local cache path for an asset file.

    Path format: <asset_id>_<file_key>.<ext>
    """
    ext_map = {
        "glb": "gltf",   # API provides gltf, not glb
        "blend": "blend",
        "fbx": "fbx",
        "obj": "obj",
        "4k_jpg": "jpg",
        "4k_exr": "exr",
        "4k_png": "png",
        "4k_normal_opengl": "jpg",
        "4k_roughness": "jpg",
        "4k_metallic": "jpg",
        "4k_ao": "jpg",
        "2k_jpg": "jpg",
        "2k_exr": "exr",
        "2k_png": "png",
        "2k_normal_opengl": "jpg",
        "2k_roughness": "jpg",
        "8k_jpg": "jpg",
        "8k_exr": "exr",
        "16k_jpg": "jpg",
        "16k_exr": "exr",
        "24k_jpg": "jpg",
        "24k_exr": "exr",
    }
    ext = ext_map.get(file_key, "dat")
    return Path(f"{asset_id}_{file_key}.{ext}")


def fetch_all(cache_dir: str, download_dir: str, all_assets: bool,
              force: bool, dry_run: bool, quiet: bool) -> list[AssetInfo]:
    """
    Fetch all assets from the curated manifest.

    Returns list of AssetInfo with status populated.
    """
    if not quiet:
        print(f"Fetching assets to {cache_dir} ...")
        if dry_run:
            print("(dry run — no files will be downloaded)")

    cache_path = Path(cache_dir)
    cache_path.mkdir(parents=True, exist_ok=True)

    assets = build_asset_list(all_assets)
    results: list[AssetInfo] = []

    for i, asset in enumerate(assets, 1):
        asset.local_path = cache_path / resolve_asset_cache(
            asset.asset_id, asset.file_key,
        )

        # Fetch file metadata from Poly Haven API.
        api_data = fetch_asset_files(asset.asset_id)
        if api_data is None:
            asset.status = DownloadStatus.NOT_FOUND
            if not quiet:
                print(f"  [{i}/{len(assets)}] ✗ {asset.asset_id}/{asset.file_key} — API error")
            results.append(asset)
            continue

        download_url = resolve_file_url(api_data, asset.file_key)
        if download_url is None:
            asset.status = DownloadStatus.NOT_FOUND
            if not quiet:
                print(f"  [{i}/{len(assets)}] ✗ {asset.asset_id}/{asset.file_key} — file not in API response")
            results.append(asset)
            continue

        asset.download_url = download_url

        # Check if file is cached and valid.
        if asset.local_path.exists() and not force:
            expected = EXPECTED_HASHES.get((asset.asset_id, asset.file_key))
            if expected and verify_md5(asset.local_path, expected):
                asset.status = DownloadStatus.UP_TO_DATE
                if not quiet:
                    print(f"  [{i}/{len(assets)}] ✓ {asset.asset_id}/{asset.file_key} — cached")
                results.append(asset)
                continue
            else:
                asset.status = DownloadStatus.HASH_MISMATCH
                # Will be re-downloaded below.

        # Download main file (or dry run).
        if dry_run:
            asset.status = DownloadStatus.DOWNLOADING
            if not quiet:
                print(f"  [{i}/{len(assets)}] → {asset.asset_id}/{asset.file_key} ({asset.download_url})")
        else:
            asset.status = DownloadStatus.DOWNLOADING
            if not quiet:
                print(f"  [{i}/{len(assets)}] ↓ {asset.asset_id}/{asset.file_key} ...", end=" ", flush=True)

            success = atomic_download(asset.download_url, asset.local_path)

            if success:
                # Verify hash if expected.
                expected = EXPECTED_HASHES.get((asset.asset_id, asset.file_key))
                if expected:
                    if verify_md5(asset.local_path, expected):
                        asset.status = DownloadStatus.SUCCESS
                        if not quiet:
                            print("✓ (hash verified)", flush=True)
                    else:
                        asset.status = DownloadStatus.HASH_MISMATCH
                        hash_actual = compute_md5(asset.local_path)
                        asset.hash_actual = hash_actual
                        logger.error(
                            "Hash mismatch for %s/%s: expected %s, got %s",
                            asset.asset_id, asset.file_key,
                            expected[:8], hash_actual[:8],
                        )
                        asset.local_path.unlink(missing_ok=True)
                        if not quiet:
                            print("✗", flush=True)
                else:
                    asset.status = DownloadStatus.SUCCESS
                    if not quiet:
                        print("✓", flush=True)
            else:
                asset.status = DownloadStatus.FAILED
                if not quiet:
                    print("✗", flush=True)

        results.append(asset)

        # For model files, also download included files (.bin buffers and textures)
        if asset.file_key == "glb":
            if not quiet:
                print(f"  [{i}/{len(assets)}] ↓ {asset.asset_id} includes ...", flush=True)
            
            include_results = download_included_files(
                api_data, asset.local_path.parent, asset.file_key,
                dry_run=dry_run, quiet=quiet
            )
            
            # Update overall status if any includes failed
            for path, md5, status in include_results:
                if status in (DownloadStatus.FAILED, DownloadStatus.HASH_MISMATCH):
                    asset.status = status
                    break

    if not quiet:
        summary = _print_summary(results)
        print(summary)

    return results


def print_hashes(cache_dir: str, md5: bool = False) -> None:
    """Print hashes for all cached assets."""
    cache_path = Path(cache_dir)
    if not cache_path.exists():
        print("Cache directory not found:", cache_dir)
        return

    hash_func = compute_md5 if md5 else compute_md5  # Default to md5 for API consistency

    for f in sorted(cache_path.iterdir()):
        if f.is_file() and not f.name.startswith("."):
            h = hash_func(f)
            print(f"{h}  {f.name}")


# ─── Summary ─────────────────────────────────────────────────────────


def _print_summary(results: list[AssetInfo]) -> str:
    """Print a summary of download results."""
    by_status: dict[DownloadStatus, list[AssetInfo]] = {}
    for r in results:
        by_status.setdefault(r.status, []).append(r)

    lines = []
    lines.append("")
    lines.append("=" * 60)
    lines.append("Fetch Summary")
    lines.append("=" * 60)
    lines.append(f"Total:    {len(results)}")

    for status in (DownloadStatus.SUCCESS, DownloadStatus.UP_TO_DATE,
                   DownloadStatus.FAILED, DownloadStatus.NOT_FOUND,
                   DownloadStatus.HASH_MISMATCH, DownloadStatus.SKIPPED):
        items = by_status.get(status, [])
        if items:
            lines.append(f"  {status.value:16s} {len(items)}")
            for item in items:
                lines.append(f"    {item.asset_id}/{item.file_key} → {item.local_path}")

    lines.append("=" * 60)
    return "\n".join(lines)


# ─── CLI ─────────────────────────────────────────────────────────────


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Mansion Desktop asset fetcher and verifier",
    )
    parser.add_argument(
        "--all", dest="all_assets", action="store_true", default=False,
        help="Fetch all assets including alternates",
    )
    parser.add_argument(
        "--force", action="store_true", default=False,
        help="Re-download even if cached and hash matches",
    )
    parser.add_argument(
        "--hashes", action="store_true", default=False,
        help="Print MD5 hashes for cached assets and exit",
    )
    parser.add_argument(
        "--cache-dir", default=None,
        help="Local cache directory (default: assets/cache)",
    )
    parser.add_argument(
        "--download-dir", default=None,
        help="Download destination (default: same as cache-dir)",
    )
    parser.add_argument(
        "--dry-run", action="store_true", default=False,
        help="Show what would be downloaded without downloading",
    )
    parser.add_argument(
        "--quiet", action="store_true", default=False,
        help="Suppress progress output",
    )
    parser.add_argument(
        "--log-level", default="INFO",
        help="Logging level (DEBUG, INFO, WARNING, ERROR)",
    )

    args = parser.parse_args()

    logging.basicConfig(
        level=getattr(logging, args.log_level.upper(), logging.INFO),
        format="%(asctime)s %(levelname)s %(message)s",
    )

    repo_root = Path(__file__).resolve().parent.parent
    default_cache = repo_root / "assets" / "cache"
    cache_dir = args.cache_dir or str(default_cache)
    download_dir = args.download_dir or cache_dir

    if args.hashes:
        print_hashes(cache_dir, md5=True)
        return 0

    results = fetch_all(
        cache_dir=cache_dir,
        download_dir=download_dir,
        all_assets=args.all_assets,
        force=args.force,
        dry_run=args.dry_run,
        quiet=args.quiet,
    )

    # Return non-zero if any downloads failed.
    failed = any(r.status == DownloadStatus.FAILED for r in results)
    not_found = any(r.status == DownloadStatus.NOT_FOUND for r in results)
    return 1 if (failed or not_found) else 0


if __name__ == "__main__":
    sys.exit(main())
