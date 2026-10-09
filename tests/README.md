# Automated tests

The Meson tests run without a display. They start `mansion-desktop --headless` on a
private socket inside a private `XDG_RUNTIME_DIR`, drive it with
`mansion-test-client` (and, in later tasks, `--input-script` files and
`--screenshot` dumps), and assert on line-based output.

Run everything:

```sh
meson compile -C build
meson test -C build --print-errorlogs
```

Run one test verbosely:

```sh
meson test -C build client-globals --verbose
```

## Layout

- `lib.sh` — shared shell helpers (`start_compositor`, `run_client`,
  `expect_line`, `wait_compositor`). Tests source it with the compositor binary
  as `$1` and the client binary as `$2`; Meson passes both.
- `mansion-test-client.c` — small C client. Prints `global <iface> <version>`,
  `connected`, `done`; with `--toplevel` it creates an xdg toplevel and prints
  `configure <w> <h>`. Later tasks add `--buffer`, `--color`, `--report-input`,
  `--egl` (see `docs/TASKS.md`). Its xdg-shell client code is generated into
  `build/` by Meson.
- `smoke_headless.sh` — P1-T01.
- `client_globals.sh` — P1-T02, P1-T03.
- `client_toplevel.sh` — P1-T03.

## Adding a test

1. Add a script `tests/<name>.sh` that sources `lib.sh` and ends with
   `echo "PASS <name>"`. Fail through `fail "..."`; it dumps compositor stderr.
2. Register it in `meson.build`:

   ```meson
   test('<name>', find_program('tests/<name>.sh'), args: [mansion_exe, test_client], suite: 'p1')
   ```

3. Keep tests deterministic: use `--exit-after-ms` budgets, never `sleep` for
   timing you can wait on with `expect_line` polling.
4. Pixel checks use `tests/ppm_pixel.py` (added in P1-T05) against
   `--screenshot` output.

Manual acceptance procedures that need a person live in `docs/ACCEPTANCE.md`
(written in P1-T10). Autonomous sessions must not report them as passed.

## Evidence boundaries and automation checks

Synthetic client events prove protocol assertions; PPM samples prove specified
pixel values. Neither proves a real terminal is usable. Real-client and human
procedures are in [ACCEPTANCE](../docs/ACCEPTANCE.md); normal/sanitizer lifetime,
commit, format and coordinate audits are the P4 recovery tasks.

Plugin checks are separate from Meson:

```sh
node --test .opencode/tests/*.test.js
```

The archive has a repository-content test assuming Projects 1–4 are perpetually
unfinished. Recovery temporarily makes it pass by adding open tasks; P4-T07 must
replace that brittle assumption with fixtures so finishing recovery cannot break
the suite. Current plugin parsing recognizes numeric IDs at column zero and
ignores human tasks; a DONE marker does not validate dependency evidence.

## Godot and graphical application checks

`tools/check-godot-binding.sh` runs native runtime/frame/input/resize and surface-
tree fixtures plus headless binding checks. `tools/check-godot-study.sh` requires
a graphical display and exercises the real terminal and room interactions.
`tools/check-chrome-application.sh` additionally requires installed Google Chrome
(`google-chrome.desktop`). It renders the real browser against a local test page,
types and clicks through Godot, scrolls, resizes, returns to the room, opens a
second window and closes it through Chrome. Missing Chrome/display is a failure.
The local page is test content, not a substitute application. Captures are under
`.tools/chrome-application-test`; the profile there is separate from regular
Mansion and host browser profiles. See [evidence and limitations](../docs/handoffs/37-graphical-applications.md).
