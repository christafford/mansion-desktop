# Automated tests

All tests run without a display. They start `mansion-desktop --headless` on a
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
  `connected`, `done`. Later tasks add `--toplevel`, `--buffer`, `--color`,
  `--report-input`, `--egl` (see `docs/TASKS.md`).
- `smoke_headless.sh` — P1-T01.
- `client_globals.sh` — P1-T02.

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
