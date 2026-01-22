# STVN
A very simple visual novel engine for Atari ST on monochrome high-res mode.

![stvn](/assets/stvn.png)

## Building
To build this, you need the following:
* GNU make
* m68k-atari-mintelf gcc toolchain
* vasmm68k_mot
* UPX for compression

## Prerequistes
* Atari ST (STe if you want to use DMA SNDH files)
* 512KiB ST-RAM or more.
* Any TOS version should work (tested only on TOS 2.06 and EmuTOS)

## Running
* Provide a script file following the documented syntax
* Edit STVN.INI to meet your needs: script file and SNDH memory buffer, which should be the size of your largest SNDH file
* Run STVN.PRG from GEM Desktop.

## Key bindings
* 'Space' to advance
* 'q' Quit
* 's' Save state
* 'l' Load state
* 'b' Go back 1 text block
* 'h' Show help screen

## STVN.INI settings
As a sample, use the following:
```
STEST.VNS
B20000
```
'S' line is the script file
'B' line is the SNDH buffer space
Defaults: ``STVN.VNS`` & ``20000``

## Supported formats / limitations:
* For pictures: PI1/PI3 monochrome, uncompressed 32KiB files.
  The palette isn't used, only the first 25600 bytes are read (640x360 image, leaving 40 pixels for text, 4 lines)

For audio: any SNDH with a working playback routine compatible with gwEm's sndhlib
Both can be gzipped.

Savestates: 4 supported, adding more would be trivial.
10 VN choices binary "registers", adding more wouldn't be very hard.
Scripts can be 999999 lines long.

All resource files must be placed in the 'data' subdirectory.

## Syntax:
Each line starts with an operand, like:
``IATARI.PI3``

Supported operands:
* 'W' : Wait for input

  This is where most interaction will happen.

  Like saving/loading, waiting for 'space', quitting, etc...

* 'I' : Change background image

  Syntax: ``I[file]`` like ``IFILE.PI3``

  Expected format is PI3, can (should) be gzipped, and is loaded entirely before being copied to video memory.

  Only the first 320 lines are loaded and drawn, the rest of the screen being used for the text area.

  A few bytes can thus be saved, for example bv removing the 400-lines padding code in pbmtopi3 source.

  As the picture is loaded in RAM before being copied in vram, this operand will use 32000 bytes of RAM during operation, freed after.

  This will obviously erase all sprites and clear the internal sprite list.

* 'S' : Sayer change

  Syntax: ``Sname`` like `SToyoyo`

  Obviously, this will reset character lines to 0.

  This is used as the save pointer and will also reset the text box.

* 'T' : Text line

  Syntax: ``Ttext`` like ``THere's a text line``

  Line wrapping is disabled, so make sure a line isn't more than 79 characters!

* 'P' : Change music

  Syntax: ``P[file]`` like ``PFILE.SND``

  Expected format is uncompressed SNDH (use unice68), but can and should be gzipped, and is loaded entirely before starting its playing routine.

  This uses gwEm's sndhlib code, which should be pretty robust, but I'm not responsible for bugged replay routines in SNDH files...

  Note: Be sure to adjust the 'B' Line in STVN.INI to match the *uncompressed* size of your largest tune. The default is 20000 bytes.

* 'J' : Jump to label

  Syntax: ``J[label]`` like `JLBL1```

  A label 4 bytes sequence at the beginning of a line

* 'C' : Offer a choice and store it in a register

  Key '1' entered: store 0

  Key '2' entered: store 1

  Key '3' entered: store 2

  Key '4' entered: store 3

  Syntax: ``C[register number][Max choices]`` like ``C04`` meaning "Get a choice between 1 and 4 and assign it to register 0"

  Also offer the same interactions as a 'W' line.

* 'R' : Redraw the background picture.

  More precisely, copy back the buffer of last loaded one into video memory.

  This will obviously erase all sprites and clear the internal sprite list.

* 'B' : Jump to label if register is set.

  Syntax: ``B[register number][Choice][label]`` like ``B02LBL1`` meaning "if register 0 is at 2, jump to LBL1"

* 'D' : Wait the specified number of milliseconds (max 99999).

  Syntax: ``D[msec]`` like ``D2000``

* 'A' : Load and draw a sprite in video memory.

  Syntax: ``A[XXX][YYY][file]`` like ``A000000TOYO.SPR``

  The expected format is derived from XPM, can (should) be gzipped and is plotted as it's read.

  A converter script is provided.

  Screen boundary are checked during drawing, but you can draw on the text area if you want.

  More precicely: X coordinates after 640 are ignored and Y coordinates after 400 ends the drawing routine.

  This can use quite some memory: 1 byte per pixel, allocated in 1024 bytes blocks, freed after operation.

  And this is still drawn slower than a full screen image.

  They're tracked as much as I could during load & back routines, so what *should* happen is:

  * If you load a save, both background & sprites ('A' lines) encoutered between the lasts 'I' or 'R' lines are drawn.
  * If you go back and neither background & sprites changes: nothing is drawn
  * If you go back and there's a background change, this will also trigger a sprite draw pass.
  * If you go back to the 'S' line just before a 'A' Line, it will trigger a draw pass too. This is by design: When the 'S' line happens, the sprite doesn't exist yet.

* 'E' : Manually erase the text box, reset character lines to 0

* 'X' : Trigger a visual effect.

  Syntax: ``X[XX]`` like ``X03``

  01: Vertically fade to black from top to bottom, 2 lines by 2 lines

  02: Vertically fade to white from top to bottom, 2 lines by 2 lines

  03: 01 then 02

  04: 02 then 01

  05: Vertically fade to black from bottom to top, 2 lines by 2 lines

  06: Vertically fade to white from bottom to top, 2 lines by 2 lines

  07: 05 then 06

  08: 06 then 05

  09: Vertically fade to black from outer to inner, 2 lines by 2 lines

  10: Vertically fade to white from outer to inner, 2 lines by 2 lines

  11: 09 then 10

  12: 10 then 09

  13: Vertically fade to black from inner to outer, 2 lines by 2 lines

  14: Vertically fade to white from inner to outer, 2 lines by 2 lines

  15: 13 then 14

  16: 14 then 13
