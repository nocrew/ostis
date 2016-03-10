# Cycle-Accurate Atari ST Emulator

### Building

To build, the SDL2 library must be installed.  Also, you need tools
compatible with Lex and Yacc.  E.g. `apt-get install libsdl2-dev
flex bison`.

There are a few different build options (always do `make clean` first):

Normal optimised binary:
`make`

Normal binary with GDB flags:
`make gdb`

Binary setup for running tests (se below):
`make test`

### Usage

Command line options:  
`ostis` [`-a` disk image] [`-b` disk image] [`-t` TOS image] [`-s` state image] [`-h`] [`-d`] [disk image] [`-p`] [`-y`] [`-V`]

`-a`  - Load disk image (.ST, .MSA or .STX)  
`-b`  - Load disk image into B: (.ST, .MSA or .STX)  
`-c`  - Mount ACSI hard drive  
`-t`  - TOS image (default: `tos.img`)  
`-s`  - Save state file  
`-h`  - Show help  
`-d`  - Activate MonST clone  
`-p`  - Dump all video into PPM file (`ostis.ppm`)  
`-y`  - Dump all audio into raw PCM file (`psg.raw`)  
`-V`  - Try to wait for 50Hz delay between frames  
`-M`  - Monochrome mode.  
`-v`  - Increase diagnostics verbosity.  
`-q`  - Decrease diagnostics verbosity.

Long options:
`--cart CARTRIDGE      ` - Load cartridge
`--loglevels LEVEL_LIST` - Set loglevels for individual modules (se below)

Special keys:  
`F11`           - save emulator state to file  
`F12`           - print diagnosics  
`F13`           - temporarily save state  
`F14`           - restore temporary state  
`PrintScr`      - toggle insane amount of debug output (live disassemble)  
`Pause`         - toggle window grab (also hides native mouse cursor)  
`Ctrl-Pause`    - toggle fullscreen

MonST keys:  
`Esc` - set window address to pc  
`TAB` - cycle window  
`Up`, `Down`, `Left`, `Right` - move window  
`D` - toggle debug mode  
`Alt-T` - change window type  
`Alt-F` - change window font  
`Alt-S` - split window  
`Ctrl-S` - skip instuction  
`Alt-Y` - toggle full screen  
`Ctrl-Z` - step  
`Ctrl-A` - run until next instruction  
`Ctrl-R` - run  
`Ctrl-M` - set window address  
`Ctrl-V` - toggle display  
`L` - set label  
`Shift-L` - Save/Load labels to/from file  
`Alt-B` - set breakpoint  
`Ctrl-B` - set breakpoint at window  
`Alt-W` - Set watchpoint  

### Logging

Levels available are (set level and everything lower):
`0` - Set to 0 to log nothing
`1` - FATAL: Fatal errors
`2` - ERROR: Normal errors
`3` - WARN:  Warnings (default)
`4` - INFO:  Information
`5` - DEBUG: Debug messages
`6` - TRACE: Trace output

`LEVEL_LIST` can be set in the following two ways:
`-IKBD` to remove all output from `IKBD`
`IKBD:2` to set `IKBD` to only log ERROR and FATAL messages

Multiple modules can be set in a comma separated string:
`-IKBD,MFP0:2,PSG0:5` means no `IKBD`, some `MFP0` and up to debugging level on `PSG0`

### Testing

At the moment of writing this, only one test exists. It is run as follows:

Build the test binary with `make test`.
Run `./ostis-test --test-case moveq`

This will load the cartridge image `tests/data/test_moveq.cart` before starting the system.  
The cartridge image contains the following code:

```
$FA0000 DC.L $FA52235F  ; Cartridge boot signature
$FA0004 BKPT #0         ; $4848 (68010 BKPT instruction)
$FA0006 MOVEQ #98,D0
$FA0008 BKPT #7         ; $484F (68010 BKPT instruction)
```

The `BKPT #0` will run the hook for test initialization (`test_moveq_hook_init()`) that can
be used to prepare things before the test

`BKPT #7` will run the exit hook (`test_moveq_hook_exit()`) which will then compare the current
state to the expected one and exit with 0 (SUCCESS) or -1 (FAIL).

The source for the cartridge images are provided alongside the .cart files. These files can be
assembled into the cartridge image using the asmx tool (or probably pretty much any other
68k assembler as well).

ASMX: http://xi6.com/projects/asmx/

To assemble:
`asmx -C 68k -b -o test_moveq.cart test_moveq.s`
