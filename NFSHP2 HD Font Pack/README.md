# NFS Hot Pursuit 2 - HD Font Pack
HD font pack for Need for Speed: Hot Pursuit 2 on PC

# Installation
- MAKE A BACKUP OF THE FRONTEND FOLDER (or just fonts.ini and gui\system\fonts.gdc)
- Extract the FrontEnd folder to the game root directory and overwrite any files it asks
- (optional) For a style change from Arial, rename MicroSquare72b.ffn to Arial72b.ffn
- Install HP2WSFix and adjust the memory (as described below)

# Requirements
- Memory adjustment - you need to install the HP2WSFix and adjust the memory size to match the following:
```ini
[MEMORY]
CLASS_UI=0xA000000
GENERAL=0x5FB90000
```
If you do not edit the memory size, the game will crash while loading a track.

# Fonts
The fonts were generated using a 72 point height for the fonts (maximum allowed currently) with the GDI engine.

- Arial
- Gulim
- MicroExtendELF (a variant of MicroSquare)

