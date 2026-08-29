# ArknightsEnhancer

A 64-bit ReShade add-on with quality-of-life features for the official Arknights PC client.

## Features

- Set an operator's deployment direction with **W/A/S/D** after placing them.
- Trigger story Skip, gacha Skip, and story confirmation Yes with **Tab** by default. Each action has its own configurable binding.
- Control story playback with **Space** for hide/show, **P** for Auto on/off, **O** for Auto speed, **Enter** to continue, and **H** for history by default.
- Toggle between windowed and fullscreen with **F12**.
- Resize the window from any edge or corner while preserving its aspect ratio. The minimum client height is 480 px.
- Control  Arknights audio from the title bar with a mute button, volume slider, or mouse wheel.
- Configure a primary and alternate key for every shortcut from the **ArknightsEnhancer** ReShade window. Left-click a key field and press a key to bind it, or right-click the field to unbind it.

The add-on uses Windows, ReShade, and the game's existing Unity UI handlers.

## Safety and account risk

This is, and always will be, a risk. ArknightsEnhancer is an unofficial modification and goes against the game's Terms of Service, regardless of how harmless the individual features may be. I cannot guarantee that it is undetectable or that using it will never result in an account penalty.

- **Lower relative risk:** Window resizing, fullscreen, and audio controls use Windows APIs and do not modify gameplay or server-side data.
- **Higher relative risk:** Shortcuts directly invoke the game's existing Unity UI functions. The gacha skip only removes the client-side delay during the character reveal; it does not change the pull result or any server-side data.

Before implementing any of this, I looked into what the current anti-cheat tracks and where server-side validation happens. I found no indication that these shortcuts or the gacha skip are being tracked. The anti-cheat appears to focus mainly on combat manipulation, such as changing operator or enemy stats, damage, deployment costs, skill timing, or game speed. Separately, server-side validation is used for actions that affect account or game state, such as currency, inventory, rewards, purchases, gacha results, and submitted battle results.

Gacha results are validated before my skip applies. The skip lock itself is completely client-side and only enforces the delay during the character reveal and voice line. This is based on the current implementation and could change with future game or anti-cheat updates.

Personally, I think it is very unlikely that these features will cause issues in the future. My previous experience with Hypergryph through Endfield shows that they generally do not care much about this kind of interaction with Unity functions. However, Arknights Global also involves Yostar, so this is only my personal assessment and not a guarantee.

**Use at your own risk.**

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

1. Download ReShade 6.7.3 or later with full add-on support from [reshade.me](https://reshade.me/downloads/ReShade_Setup_6.8.0_Addon.exe) and run the ReShade installer.
2. Select the Arknights executable. For the default installation, it is usually:
   ```text
   C:\YostarGames\Arknights_EN\Arknights.exe
   ```
3. Select the rendering API used by the client: **DirectX 10/11/12**.
4. Finish the installation. ReShade places its proxy DLL beside the selected game executable.
5. Copy `ArknightsEnhancer.addon64` into the same directory, beside both the game executable and the ReShade proxy DLL.

The default installation should contain the files:
```te
C:\YostarGames\Arknights_EN\Arknights.exe
C:\YostarGames\Arknights_EN\dxgi.dll
C:\YostarGames\Arknights_EN\ArknightsEnhancer.addon64
```
## Credits and license

Created by **ItsTheSewerRat**. The WASD interaction is based on [ACK72/THRM-EX](https://github.com/ACK72/THRM-EX).

Released under the [MIT License](LICENSE).
