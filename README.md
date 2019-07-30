SourcePauseTool
===============

A plugin for all your pausing needs.

## Usage
1. Download the DLL corresponding to your game / engine:
    * Source SDK 2013 / SteamPipe - `spt-2013.dll`
    * Black Mesa - `spt-bms.dll`
    * Source SDK 2006 / Old Engine - `spt-oe.dll`
    * Portal 2 - `spt-p2.dll`
    * Source SDK 2007 / Source Unpack / New Engine - `spt.dll`

2. Place the DLL into the correct folder:
    * **For Old Engine** you will need to place `spt-oe.dll` into the topmost `bin` directory-the one with lots of different DLL files including `engine.dll`, **not** the one with `client.dll` and `server.dll`.
    * **For Old Engine *mods***, the correct `bin` folder is in the `Source SDK Base` directory.
    * Half-Life 2 - `hl2`
    * Half-Life 2: Episode 1 - `episodic`
    * Half-Life 2: Episode 2 - `ep2`
    * Portal - `portal`
    * Portal 2 - `portal2`
    * Black Mesa - `bms`

3. Launch the game.
4. Go to `Options > Keyboard > Advanced`, check `Enable developer console`, and press OK.
5. Press the tilde key (~) and enter `plugin_load spt` into the developer console.

   To make SourcePauseTool load automatically add `plugin_load spt` to `cfg/autoexec.cfg`.

## Building
You will need Visual Studio 2017 or later, and [git](https://git-scm.com).

1. Enter `git clone -b %SDK% https://github.com/alliedmodders/hl2sdk.git` into cmd, replacing `%SDK%` with the correct SDK:
    * Source SDK 2007 (Source Unpack/New Engine/Orange Box) - `orangebox`
    * Black Mesa - `bms`
    * Source SDK 2013 (SteamPipe) - `sdk2013`
    * Source SDK 2006 (Old Engine) - `episode1`
    * Portal 2 - `portal2`
    
   For example, `git clone -b orangebox https://github.com/alliedmodders/hl2sdk.git`

2. Enter the following into cmd:
    ```
    git clone --recurse-submodules https://github.com/YaLTeR/SourcePauseTool.git hl2sdk\utils\SourcePauseTool
    
    hl2sdk\utils\SourcePauseTool\spt.sln
    ```

3. If Windows asks which program to open `spt.sln` with, choose Visual Studio.

   If Visual Studio asks to retarget projects, press OK.

   Once Visual Studio is open, right click `libMinHook`, click `Properties`, change `Platform Toolset` to the one corresponding to your Visual Studio version, and press OK.

4. Choose the build configuration corresponding to the SDK you cloned:
    * Source SDK 2007 (Source Unpack/New Engine/Orange Box) - `Release`
    * Source SDK 2013 (SteamPipe) - `Release 2013`
    * Black Mesa `Release BMS`
    * Source SDK 2006 (Old Engine) `Release OE`
    * Portal 2 - `Release P2`

5. Click `Build > Build Solution`.

   `spt*.dll` will be in `hl2sdk\utils\SourcePauseTool\<Build Configuration>`

## .srctas documentation
.srctas is the SPT TAS script format. You can find its documentation [here](https://docs.google.com/document/d/11iu9kw5Ufa3-QaiR7poJWBwfe1I56wI6fBtDgmWZ8Aw).
