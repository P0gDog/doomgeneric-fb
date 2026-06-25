# doomgeneric-fb

A fork of [doomgeneric](https://github.com/ozkl/doomgeneric) with a Linux framebuffer (`/dev/fb0`) + evdev keyboard backend, designed to run as PID 1 on a bare Linux kernel with no display server. See https://github.com/P0gDog/DoomOS to see this as a working OS.

## Files added/modified
- `doomgeneric_fb.c` - framebuffer + evdev input backend

## Building
```bash
sudo apt install -y gcc build-essential linux-libc-dev
make CC=gcc CFLAGS="-static -O2" LDFLAGS="-static"
```

## License
[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://opensource.org/license/gpl-2.0)

