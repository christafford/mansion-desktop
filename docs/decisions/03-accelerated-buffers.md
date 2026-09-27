# Decision 03 — Accelerated buffer sharing

Date: 2026-09-27
Related tasks: P2-T05 (Accelerated client experiment)

## Question

Can the compositor import a client's EGL-rendered buffer directly using
`EGL_WL_bind_wayland_display` or `zwp_linux_dmabuf_v1`?

## Findings

### EGL + Wayland probe

Connected a client to the compositor's private Wayland socket (`egl-test`)
and initialized EGL with `EGL_PLATFORM_WAYLAND_KHR`:

- `eglBindWaylandDisplayWL`: **NULL** (not available)
- `eglUnbindWaylandDisplayWL`: **NULL** (not available)
- `eglQueryWaylandBufferWL`: **NULL** (not available)

These three symbols are required for the `EGL_WL_bind_wayland_display`
extension to work. The Mesa EGL driver in this container does not load
the Wayland buffer-binding functions, even when the display is
`EGL_PLATFORM_WAYLAND_KHR`.

### EGL + DMA-BUF export probe

- `EGL_MESA_image_dma_buf_export` extension: **available**
- `eglCreateImageKHR` symbol via `dlsym("libEGL.so.1", ...)`: **NULL**
- `eglQueryImageKHR` symbol: **NULL**
- `eglDestroyImageKHR` symbol: **NULL**

The EGL image creation/export symbols are not accessible through the
dlsym path even though the extension string reports the extension.
This is consistent with Mesa's surfaceless driver which provides
 EGL images internally but does not expose the KHR_image API for
direct use by client code.

### EGL + PBuffer rendering

EGL PBuffer rendering **does work**:

- `eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA)` → success
- `eglCreatePbufferSurface(256x256)` → success
- `glClearColor(0,1,0); glClear()` → green screen
- `glReadPixels` → confirms R=0 G=255 B=0 (correct)

The GPU renders correctly to the PBuffer, but reading back requires
`glReadPixels` which copies pixels through the CPU.

### GL renderer

Mesa radeonsi (vangogh) on Steam Deck — `/dev/dri/renderD128` is
world-accessible.

## Conclusion

**`EGL_WL_bind_wayland_display` is not available in this environment.**
The Mesa EGL driver does not provide the Wayland buffer-binding
functions, likely because it is built against a surfaceless Mesa
configuration that does not include the Wayland EGL platform driver.

**`eglCreateImageKHR` / DMA-BUF export symbols are also not accessible**
through the standard dlsym path, even though the extension string
reports support for `EGL_MESA_image_dma_buf_export`.

### Mitigation

The `--egl` test client renders to an EGL PBuffer, reads pixels with
`glReadPixels` (RGBA), writes them to a memfd-backed `wl_shm` buffer,
and the compositor uploads the buffer as a texture. The compositor
skips the normal ARGB→RGBA swizzle when `--egl` is used.

This path demonstrates that:

1. GPU rendering via EGL works (PBuffer produces correct colours).
2. The compositor can accept and render the resulting buffer.
3. The full pipeline (EGL render → shm export → compositor upload)
   functions end-to-end.

The buffer is **not** shared zero-copy — the client copies pixels from
GPU to CPU via `glReadPixels`. For true accelerated buffer sharing
in production, a Wayland EGL platform driver that exports
`eglQueryWaylandBufferWL` or `zwp_linux_dmabuf_v1` support would be
required.
