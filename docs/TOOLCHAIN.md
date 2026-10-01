# Pinned toolchain for Project 21

Pinned 2026-10-01. All releases are the latest stable at time of pinning.
Checksums are from official sources; verify before use.

## Target hardware

- **Device:** Steam Deck (SteamOS)
- **CPU:** AMD Custom APU 0405 (Zen 2, 4 cores / 8 threads, 2.4–3.5 GHz)
- **GPU:** RDNA 2, 8 CUs (1.0 GHz) — Vulkan 1.3 support
- **RAM:** 16 GB LPDDR5
- **Display:** 1280×800 LCD
- **OS:** SteamOS (Arch Linux-based)

## Performance goals

- Sustained 30 FPS at 1280×800 on Steam Deck (minimum target)
- Pursue 60 FPS where quality/performance tradeoff permits
- Record percentile frame times and worst scene, not just average FPS
- Record hardware/resolution/renderer for all measurements

## Renderer candidates

1. **Compatibility renderer (Vulkan)** — broadest compatibility, best feature set.
   Primary choice for Steam Deck.
2. **Mobile renderer** — lower GPU cost, fewer features. Compare against
   Compatibility using actual scenes on target hardware.
3. **GLES3** — fallback if Vulkan unavailable; not expected on Steam Deck.

Decision: start with Compatibility (Vulkan). Compare Mobile after the first
room passes visual acceptance.

## Godot Engine

| Field | Value |
|---|---|
| Version | 4.7.2-stable |
| Release date | 2026-08-18 |
| Source | <https://github.com/godotengine/godot/releases/tag/4.7.2-stable> |
| Source download | <https://github.com/godotengine/godot/releases/download/4.7.2-stable/godot-4.7.2-stable.tar.xz> |
| Source archive size | 45.5 MB |
| Source SHA-256 | `a18ce0ccec3ecc40b0dd6c4f5132ca934e9fb7c2979717940ff32aee1eb35481` |
| Editor binary download | <https://github.com/godotengine/godot/releases/download/4.7.2-stable/Godot_v4.7.2-stable_linux.x86_64.zip> |
| License | MIT |
| License URL | https://github.com/godotengine/godot/blob/4.7.2-stable/LICENSE.txt |

### Source (verified, repository-local)

```sh
mkdir -p tools
cd tools
wget -O godot-4.7.2-stable.tar.xz \
  https://github.com/godotengine/godot/releases/download/4.7.2-stable/godot-4.7.2-stable.tar.xz
echo "a18ce0ccec3ecc40b0dd6c4f5132ca934e9fb7c2979717940ff32aee1eb35481  godot-4.7.2-stable.tar.xz" | sha256sum -c
tar xf godot-4.7.2-stable.tar.xz
rm godot-4.7.2-stable.tar.xz
```

### Editor binary (must be downloaded separately)

The pre-built editor binary is a separate download from the source tarball:

```sh
cd tools
wget -O Godot_v4.7.2-stable_linux.x86_64.zip \
  https://github.com/godotengine/godot/releases/download/4.7.2-stable/Godot_v4.7.2-stable_linux.x86_64.zip
unzip Godot_v4.7.2-stable_linux.x86_64.zip
rm Godot_v4.7.2-stable_linux.x86_64.zip
```

### Run

```sh
tools/Godot_v4.7.2-stable_linux.x86_64 --headless --version
```

## godot-cpp (GDExtension bindings)

| Field | Value |
|---|---|
| Version | 10.0.0-stable |
| Release date | 2026-09-15 |
| Source | <https://github.com/godotengine/godot-cpp/releases/tag/10.0.0-stable> |
| Download | <https://github.com/godotengine/godot-cpp/archive/refs/tags/10.0.0-stable.tar.gz> |
| SHA-256 | _(verify from GitHub release page before use)_ |
| License | MIT |
| License URL | https://github.com/godotengine/godot-cpp/blob/10.0.0-stable/LICENSE |

### Build (requires Godot editor or template binaries in `GODOT_BINARIES`)

```sh
git clone --branch 10.0.0-stable --depth 1 \
  https://github.com/godotengine/godot-cpp.git tools/godot-cpp
cd tools/godot-cpp
scons generate_bindings=yes platform=linux generate_headers=yes \
  target=template_debug arch=x86_64
```

Alternative with SCons (no generate_bindings step needed if `GODOT_BINARIES`
is set):

```sh
cd tools/godot-cpp
GODOT_BINARIES=$PWD/../godot-4.7.2-stable scons platform=linux \
  target=template_debug arch=x86_64
```

## Blender

| Field | Value |
|---|---|
| Version | 5.2.2 LTS |
| Release date | 2026-09-15 |
| Source | <https://www.blender.org/download/releases/5-2/> |
| Linux download | <https://download.blender.org/release/Blender5.2/blender-5.2.2-linux-x64.tar.xz> |
| Archive size | 366 MB |
| License | CC0 (Blender) / Apache 2.0 (source) |
| License URL | https://www.blender.org/about/license/ |

### Install (repository-local)

```sh
cd tools
wget -O blender-5.2.2-linux-x64.tar.xz \
  https://download.blender.org/release/Blender5.2/blender-5.2.2-linux-x64.tar.xz
# Verify SHA-256 from https://download.blender.org/release/Blender5.2/blender-5.2.1.sha256
tar xf blender-5.2.2-linux-x64.tar.xz
rm blender-5.2.2-linux-x64.tar.xz
```

### Headless GLB export (batch conversion)

```sh
tools/blender-5.2.2-linux-x64/blender --background --python convert.py \
  -- input.blend output.glb
```

## Host build dependencies (Arch Linux)

Already present in the `mansion-dev` container:

| Tool | Version | Purpose |
|---|---|---|
| meson | 1.x | Build system |
| ninja | 1.13 | Build backend |
| g++ | 16.2.1 (C++20) | Compiler |
| pkg-config | — | Dependency discovery |
| wayland-scanner | 1.26.0 | Wayland protocol generation |
| node | 26.x | Plugin tests |
| python3 | — | Test scripts |

| Library | Version | Purpose |
|---|---|---|
| wayland-server | 1.26.0 | Wayland compositor core |
| wayland-client | 1.26.0 | Test client |
| wayland-protocols | — | xdg-shell, relative-pointer |
| wayland-egl | 18.1.0 | EGL Wayland window system |
| egl | 1.5 | EGL API |
| glesv2 | 3.2 | OpenGL ES 2 |
| xkbcommon | 1.13.2 | Keyboard handling |
| x11 | 1.8.13 | Declared in meson.build but no longer linked (dead dependency, P4-T06) |

## Asset download tools

Already present in the container:

| Tool | Purpose |
|---|---|
| wget | HTTP/HTTPS downloads |
| curl | HTTP/HTTPS downloads |
| sha256sum | Checksum verification |
| unzip | Archive extraction |
| tar | Archive extraction |
| file | File type detection |

## Poly Haven asset access

- API: <https://api.polyhaven.com/>
- Terms: <https://github.com/Poly-Haven/Public-API/blob/master/ToS.md>
- License: CC0 — <https://polyhaven.com/license>
- All assets are public domain; no API key required

## Offline behavior

- Godot, godot-cpp, and Blender are downloaded once into `tools/` and cached
  in the repository. The bootstrap script should check for their presence and
  only download if missing.
- Asset downloads from Poly Haven use a local manifest for identifying
  verified files. Missing files produce actionable errors, not silent fallbacks.
- The Godot editor and Blender are needed for authoring/conversion only;
  runtime Godot can use the headless template binary.
