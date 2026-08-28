# ArknightsEnhancer

A 64-bit ReShade add-on with quality-of-life features for the official Arknights PC client.

## Features

- Set an operator's deployment direction with **W/A/S/D** after placing them.
- Press the in-game **Skip** button with **Tab**.
- Toggle between windowed and fullscreen with **F12**.
- Resize the window with a corner grip. The minimum client height is 480 px.
- Control only Arknights audio from the title bar with a mute button, volume slider, or mouse wheel.
- Configure a primary and alternate key for every shortcut from the **ArknightsEnhancer** ReShade window. Left-click a key field and press a key to bind it, or right-click the field to unbind it.

The add-on uses Windows and ReShade APIs. It does not read or modify Unity game memory.

## Requirements

- Windows x64
- Official Arknights PC client
- ReShade 6.7.3 or later with full add-on support

## Build

Install Visual Studio with **Desktop development with C++**, then run:

```powershell
.\build.ps1
```

The output is `build\Release\ArknightsEnhancer.addon64`. The build script downloads the ReShade 6.7.3 SDK if needed.

## Install

Copy `ArknightsEnhancer.addon64` beside `Arknights.exe` and the ReShade proxy DLL. For the default installation:

```text
C:\YostarGames\Arknights_EN\ArknightsEnhancer.addon64
```

## Credits and license

Created by **ItsTheSewerRat**. The WASD interaction is based on [ACK72/THRM-EX](https://github.com/ACK72/THRM-EX).

Released under the [MIT License](LICENSE).
