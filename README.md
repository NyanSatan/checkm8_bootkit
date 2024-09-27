# checkm8_bootkit

Little utility to boot **iBoot** on some **checkm8**-able platforms

It doesn't require any modifications to **ipwndfu**/**gaster**/etc. shellcodes since it utilizes **ipwndfu**'s [custom protocol](https://github.com/axi0mX/ipwndfu/blob/master/src/usb_0xA1_2_armv7.S)

## SoC support

* **S5L8950X** - Apple A6
* **S5L8947X** - Apple A5 (single-core)
* **S5L8747X** - Haywire SoC

* **S7002** - Apple S1
* **T8002** - Apple S1P/S2/T1
* **T8004** - Apple S3

## Usage

`bootloader` is a path to raw unpacked **iBoot** image (usually you'd want to load **iBSS**)

```
➜  checkm8_bootkit git:(master) ✗ build/checkm8_bootkit
usage: build/checkm8_bootkit <bootloader> [--debug]
supported platforms:
        s5l8950x, s5l8747x, s5l8947x
supported watch platforms:
        s7002, t8002, t8004
```

## Building

Requirements:

* libirecovery and its' dependencies
* [vmacho](https://github.com/Siguza/misc/blob/master/vmacho.c)
    * Only needed if you want to rebuild the payloads

Then just use `make`:

```
➜  checkm8_bootkit git:(master) ✗ make
        building C: src/main.c
        building C: src/libbootkit/libbootkit.c
        building C: src/libbootkit/libbootkit_watch.c
        linking
%%%%% done building
```

## TODOs

* iOS port to use with Haywire (or whatever)
