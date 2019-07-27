SourcePauseTool
===============

A plugin for all your pausing needs.

## Usage
*Note: Steam, by default, installs games to `C:\Program Files (x86)\Steam\steamapps\common` on 64-bit Windows, and `C:\Program Files\Steam\steamapps\common` on 32-bit Windows.*

1. Place the corresponding `spt*.dll` in the mod directory, *for example*:
  * `spt-2013.dll` - Source SDK 2013/SteamPipe.
  * `spt-bms.dll` - Black Mesa
  * `spt-oe.dll` - Source SDK 2006/Old Engine.
  * `spt-p2.dll` - Portal 2
  * `spt.dll` - Source SDK 2007/Source Unpack/New Engine.
* Goes in:
  * `hl2` - Half-Life 2
  * `episodic` - Half-Life 2: Episode 1
    * For Old Engine you will need to place `spt-oe.dll` into the topmost `bin` directory; The one with a lot of different DLL files including `engine.dll`, **not** the one with `client.dll` and `server.dll`.
  * `ep2` - Half-Life 2: Episode 2
  * `portal` - Portal
  * `portal2` - Portal 2
  * `bms` - Black Mesa

2. Launch the game.
3. Go to `Settings > Keyboard > Advanced` and check `Enable Developer Console`.
4. Press the tilde key (~) and enter `plugin_load spt` into the developer console. 
    * To automatically load SourcePauseTool add `plugin_load spt` to the `cfg/autoexec.cfg`.

## Building
You will need Visual Studio 2017 or later, and [Git](https://git-scm.com).

1. Enter the following into cmd:
```
git clone -b <SDK> https://github.com/alliedmodders/hl2sdk.git
# Replace <SDK> with the wanted SDK:
# orangebox - Source SDK 2007 (Source Unpack/New Engine/Orange Box)
# bms - Black Mesa
# sdk2013 - Source SDK 2013 (SteamPipe)
# episode1 - Source SDK 2006 (Old Engine)
# portal2 - Portal 2
git clone --recurse-submodules https://github.com/YaLTeR/SourcePauseTool.git hl2sdk\utils\SourcePauseTool
hl2sdk\utils\SourcePauseTool\spt.sln
```
1.5:
*If Windows asks which program to open `spt.sln` with, choose Visual Studio.*
* *If Visual Studio asks to retarget projects press OK.*
  * *Once Visual Studio is open, right click `libMinHook`, change Platform Toolset to the correct one, and press OK.*

2. Choose the build configuration corresponding to the SDK you cloned:
* `Release` - Source SDK 2007 (Source Unpack/New Engine/Orange Box)
* `Release 2013` - Source SDK 2013 (SteamPipe)
* `Release BMS` - Black Mesa
* `Release OE` - Source SDK 2006 (Old Engine)
* `Release P2` - Portal 2

3. Press *Ctrl+Shift+B* to build SourcePauseTool.

`spt*.dll` will be in `hl2sdk\utils\SourcePauseTool\<Configuration>`

## .srctas documentation
.srctas is the SPT TAS script format. You can find its documentation [here](https://docs.google.com/document/d/11iu9kw5Ufa3-QaiR7poJWBwfe1I56wI6fBtDgmWZ8Aw/edit).
