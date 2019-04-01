SourcePauseTool
===============

A plugin for all your pausing needs.

## Usage
Place the compiled `spt.dll` into your mod directory (for example: `hl2`, `portal`, `ep2`), launch the game and type `plugin_load spt` into the developer console. For older engines you will need to place `spt.dll` into the topmost `bin` directory (the one with a lot of different DLL files including `engine.dll`, *not* the one with `client.dll` and `server.dll`).

## Building
You will need Visual Studio 2017 or above.
1. Get the Source SDK code that you wish to build SPT for.
1. Clone this repository into a folder under `src\utils`, where `src` is the folder with the Source SDK source code. The files from this repository should end up inside `src\utils\SourcePauseTool`.
1. Inside the `SourcePauseTool` folder do `git submodule update --init --recursive`.
1. Open `spt.sln` in Visual Studio and build the correct build configuration:
   - Release OE for Source SDK 2006
   - Release for Source SDK 2007
   - Release 2013 for Source SDK 2013
   - Release P2 for the Portal 2 SDK from alliedmodders.

## .srctas documentation
.srctas is the SPT TAS script format. You can find its documentation [here](https://docs.google.com/document/d/11iu9kw5Ufa3-QaiR7poJWBwfe1I56wI6fBtDgmWZ8Aw/edit?usp=sharing).
