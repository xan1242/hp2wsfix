# HP2WSFix
A rudimentary widescreen fix for Need for Speed: Hot Pursuit 2 on PC

# Status
- Recalculates camera FOV
- Fixes ingame and FrontEnd resolution
- FrontEnd scaling currently WIP (scales to nearest 4:3 width on the center of the screen)
- Configure and increase the memory allocation for the render class (CLASS_RENDER) - this fixes higher poly models from crashing the game (e.g. car mods)
- Reroute the save directory (currently mandatory, will not be in the future)

# Usage
Extract the contents of the .zip file to the game's root directory.

**TEMPORARY** - transfer your save files to the "save" directory in the game's root (this will not be necessary at a later point!)

To test the new FE scaling mode, set FixHUD to 2 in the ini.

# Downloads
You should be able to download the files necessary in the Releases tab.

# Credits
- ThirteenAG - for the injector, inireader and the Ultimate ASI Loader.
- EA Canada - the game code itself
