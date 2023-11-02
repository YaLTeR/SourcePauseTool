# SourcePauseTool
[![Actions Status](https://github.com/YaLTeR/SourcePauseTool/workflows/CI/badge.svg?branch=master)](https://github.com/YaLTeR/SourcePauseTool/actions?query=branch%3Amaster)

A plugin for all your pausing needs.

### Usage
1. Download the DLL corresponding to your game / engine / SDK:
    SDK                                                               | SPT DLL
    ----------------------------------------------------------------- | --------------
    Source SDK (Base) 2007 / New Engine / Source Unpack* / Orange Box | `spt.dll`
    Source SDK (Base) 2013 / SteamPipe / Latest                       | `spt-2013.dll`
    Black Mesa                                                        | `spt-bms.dll`
    Source SDK (Base) 2006 / Old Engine                               | `spt-oe.dll`
    
    \* Including Portal 3420

2. Place the DLL into the correct folder:
    SDK                        | Folder
    -------------------------- | -----------------------------------------
    Old Engine & Portal (3420) | Topmost `bin` with `AdminServer.dll` etc.
    Old Engine mods            | `Source SDK Base\bin`
    Half-Life: Source          | `hl1`
    Half-Life 2                | `hl2`
    Half-Life 2: Episode 1     | `episodic`
    Half-Life 2: Episode 2     | `ep2`
    Half-Life 2: Lost Coast    | `lostcoast`
    Portal                     | `portal`
    Black Mesa                 | `bms`
3. Launch the game.
4. Go to `Options > Keyboard > Advanced`, check `Enable developer console`, and press OK.
5. Press the tilde key (<kbd>~</kbd>) and enter `plugin_load spt` into the developer console.
    * You must `disconnect` first in Black Mesa.
    * Add `plugin_load spt` to `cfg/autoexec.cfg` to load SourcePauseTool automatically.

### Building
0. You will need [Visual Studio 2022](https://visualstudio.microsoft.com/free-developer-offers/) (or later) and [git](https://git-scm.com).
1. Open Visual Studio 2022. Click on Tools → Get Tools and Features... from the top bar of the window.
<br>This should open the Visual Studio Installer in another window. From the Workload tab, install `Desktop development with C++`. From the Individual Components tab, install:
    * MSVC v143 - VS 2022 C++ x64/x86 build tools
2. Run the following in cmd:
    ```
    git clone --recurse-submodules https://github.com/YaLTeR/SourcePauseTool.git
    SourcePauseTool\spt.sln
    ```
3. If Windows asks which program to open `spt.sln` with, choose Visual Studio.
    1. If Visual Studio asks to retarget projects, press OK.
    2. Once Visual Studio is open, right click `libMinHook`, click `Properties`, change `Platform Toolset` to the one corresponding to your Visual Studio version, and press OK.
4. Choose the build configuration:
    SDK                                                       | Configuration
    --------------------------------------------------------- | -------------
    Source SDK 2007 / New Engine / Source Unpack / Orange Box | `Release`
    Source SDK 2013 / SteamPipe / Latest                      | `Release 2013`
    Black Mesa                                                | `Release BMS`
    Source SDK 2006 / Old Engine                              | `Release OE`
5. Click `Build > Build Solution`.
<br>`spt*.dll` will be in `hl2sdk\utils\SourcePauseTool\<Build Configuration>`

### .srctas documentation
.srctas is the SPT TAS script format. You can find its documentation [here](https://docs.google.com/document/d/11iu9kw5Ufa3-QaiR7poJWBwfe1I56wI6fBtDgmWZ8Aw).

### Contributing
See the contributing guide [here](CONTRIBUTING.md).
