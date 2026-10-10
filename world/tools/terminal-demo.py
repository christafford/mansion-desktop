#!/usr/bin/env python3
"""Ordinary terminal program for observing live output; no Elsewhere client API."""
import datetime
import os
import time

print("\033[2J\033[H\033[1;36mELSEWHERE  /  LIVE TERMINAL\033[0m\n")
print("Weston terminal  •  native Wayland client")
print(f"Linux {os.uname().release}\n")
print("This text is rendered by the terminal process.")
print("The clock and counter below update every second.\n")
print("\033[31mRED\033[0m   \033[32mGREEN\033[0m   \033[34mBLUE\033[0m   \033[37mWHITE\033[0m\n")
print("Output preview — keyboard control comes next.\n")
for tick in range(86400):
    stamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%d  %H:%M:%S UTC")
    print(f"\r\033[K{stamp}   |   update {tick:05d}", end="", flush=True)
    time.sleep(1)
