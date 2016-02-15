# Cycle-Accurate Atari ST Emulator

### Building

To build, the SDL2 library must be installed.  Also, you need tools
compatible with Lex and Yacc.  E.g. `apt-get install libsdl2-dev
flex bison`.

There are a few different build options:

Normal optimised binary:
`make`

Normal binary with GDB flags:
`make gdb`


### Usage

Command line options:  
`ostis` [`-a` disk image] [`-t` TOS image] [`-s` state image] [`-h`] [`-d`] [disk image] [`-p`] [`-y`] [`-V`]

`-a`  - Load disk image (.ST or .MSA)  
`-t`  - TOS image (default: `tos.img`)  
`-s`  - Save state file  
`-h`  - Show help  
`-d`  - Activate MonST clone  
`-p`  - Dump all video into PPM file (`ostis.ppm`)  
`-y`  - Dump all audio into raw PCM file (`psg.raw`)  
`-V`  - Try to wait for 50Hz delay between frames

Special keys:  
`F11`      - save emulator state to file  
`F12`      - print diagnosics  
`F13`      - temporarily save state  
`F14`      - restore temporary state
`PrintScr` - toggle insane amount of debug output (live disassemble)

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
`Alt-B` - set breakpoint  
`Ctrl-B` - set breakpoint at window  
`Alt-W` - Set watchpoint  
