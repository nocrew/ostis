# Cyle-Accurate Atari ST Emulator

### Building

To build, the SDL library must be installed.  E.g. `apt-get install libsdl1.2-dev`.

Set `-DDEBUG=1` in `CFLAGS` to boot into a MonST clone.

### Usage

Command line options:  
`ostis` [`-a` disk image] [`-t` TOS image] [`-s` state image] [`-h`] [disk image]

Special keys:  
`F11` - save emulator state to file  
`F12` - print diagnosics  
`F13` - temporarily save state  
`F14` - restore temporary state  

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
