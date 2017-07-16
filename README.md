SourcePauseTool
===============

A plugin for all your pausing needs.

## Usage
Place the compiled `spt.dll` into your mod directory (for example: `hl2`, `portal`, `ep2`), launch the game and type `plugin_load spt` into the developer console. For older engines you will need to place `spt.dll` into the topmost `bin` directory (the one with a lot of different DLL files including `engine.dll`, *not* the one with `client.dll` and `server.dll`).

## Building
You will need Detours v3.0 (the Express version will do), the Source SDK and Visual Studio 2015 or above. Clone the repository into a folder under `src\utils\`, where `src` is the folder with the mod source code. Do `git submodule update --init --recursive`. Configure VC++ Directories so that you have your Detours `includes` and `libs` directories listed appropriately. Chose the build configuration:
- Release OE for Source SDK 2006
- Release for Source SDK 2007
- Release 2013 for Source SDK 2013