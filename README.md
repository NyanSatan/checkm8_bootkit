# checkm8_bootkit

Little utility to boot **iBoot** on some **checkm8**-able platforms. Now also can decrypt KBAGs and demote

It doesn't require any modifications to **ipwndfu**/**gaster**/etc. shellcodes since it utilizes **ipwndfu**'s [custom protocol](https://github.com/axi0mX/ipwndfu/blob/master/src/usb_0xA1_2_armv7.S)

You can run it on iOS as well (if you are lucky)

## SoC support

* **S5L8747X** - Haywire SoC
* **S5L8940X** - Apple A5
* **S5L8942X** - Apple A5 (32nm)
* **S5L8945X** - Apple A5X - untested
* **S5L8947X** - Apple A5 (single-core)
* **S5L8950X** - Apple A6

* **S7002** - Apple S1
* **T8002** - Apple S1P/S2/T1
* **T8004** - Apple S3

## Usage

```
➜  checkm8_bootkit git:(full) ✗ build/checkm8_bootkit                                                                                                        
usage: build/checkm8_bootkit VERB [args]
where VERB is one of the following:
        boot <bootloader>
        kbag <kbag>
        demote

supported platforms:
        s5l8747x, s5l8940x, s5l8942x, s5l8945x, s5l8947x, s5l8950x, s7002, t8002, t8004
```

* `bootloader` must be a path to raw unpacked **iBoot** image (usually you'd want to load **iBSS**)
* `kbag` must be a hex string

## Building

Requirements:

* [lilirecovery](https://github.com/NyanSatan/lilirecovery)
    * My little libirecovery fork
    * Included as a Git module

* [vmacho](https://github.com/Siguza/misc/blob/master/vmacho.c)
    * Only needed if you want to rebuild the payloads

Then just use `make`:

```
➜  checkm8_bootkit git:(full) ✗ make      
        building checkm8_bootkit for Mac
        building checkm8_bootkit for iOS
%%%%% done building
```
