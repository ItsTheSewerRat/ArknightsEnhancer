# ArknightsEnhancer

A 64-bit ReShade add-on with quality-of-life features for the official Arknights PC client.

## Features

- Set an operator's deployment direction with **W/A/S/D** after placing them.
- Trigger story Skip, gacha Skip, and story confirmation Yes with **Tab** by default. Each action has its own configurable binding.
- Control story playback with **Space** for hide/show, **P** for Auto on/off, **O** for Auto speed, **Enter** to continue, and **H** for history by default.
- Toggle between windowed and fullscreen with **F12**.
- Resize the window from any edge or corner while preserving its aspect ratio. The minimum client height is 480 px.
- Control only Arknights audio from the title bar with a mute button, volume slider, or mouse wheel.
- Configure a primary and alternate key for every shortcut from the **ArknightsEnhancer** ReShade window. Left-click a key field and press a key to bind it, or right-click the field to unbind it.

The add-on uses Windows, ReShade, and the game's existing Unity UI handlers.

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
