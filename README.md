# Alienfx tools
Main goal of this project is to create a bunch of light weighted tools for Alienware AlienFX/Dell LightFX controls:
- alienfx-probe - Looking up for low-level devices, shows it's IDs, then check it and define device and light names.
- alienfx-cli - Make changes and check status of your AlienFX/LightFX lights from the command line.
- AlienFX Universal haptics - Visualize any sound around you (microphone, audio player, game, movie).
- AlienFX Ambient lights - Visualize screen picture as ambient light (from desktop, game, video player).
- AlienFX GUI Editor - Light weighted light control tool (AWCC alternative). You can change lights color according to system monitoring values (CPU/GPU load, temperatures, etc), active or running application, AC or battery power and a lot more.

## Requirements
- Alienware light device present into the system and have USBHID driver active.
- `alienfx-ambient` uses DirectX for screen capturing, so you need to download and install it from [here](https://www.microsoft.com/en-us/download/details.aspx?id=35).
- (Optional) For `alienfx-cli` and `alienfx-probe` high-level support, Alienware LightFX DLLs should be installed on your computer. These are automatically installed with Alienware Command Center and should be picked up by this program. You also should enable Alienfx API into AWCC to utilize high-level access: Settings-Misc at Metro version (new), right button context menu then "Allow 3rd-party applications" in older Desktop version 
- Windows 10 (binary files for x64 only, but you can compile project for x86 as well).

Device checked: `Alienware m15R1-R4` (API v3), `Alienware m17R1` (API v3), `Alienware M13R2` (API v2), `Dell G5` (API v3), `Alienware M14x` (API v1) (should work with any Alienware device with API v1 or later, per-button light keyboard devices and some external devices like mouses doesn't supported now).

## Known issues
- Per-button light keyboard devices (API v4, 64 bytes command) does not supported (i'm working on it). But you still can control other lights (logo, power).
- External devices (mouse, display) not supported - they have different vendor ID (i'm working on it).
- Some High-level (Dell) SDK functions doesn't work as designed. This may be fixed in upcoming AWCC updates.
- `alienfx-cli` `set-zone` and `set-zone-action` commands not supported with low-level SDK (no zones defined).
- Hardware light effects morph, breathing, spectrum, rainbow doesn't supported for older (v1, v2) devices.
- Hardware light effects can't work with software light effects at the same time (hardware bug, "Update" command stop all effects).
- DirectX12 games didn't allow to access GPU or frame, so `alienfx-ambient` didn't work, and `alienfx-gui` can't handle GPU load for it correctly.
- **WARNING!** Strongly recommended to stop AWCCService if you plan to use gui, haptics or ambient application. Keep it working can provide unexpected results, especially if you handle Power Button in gui app.
- **WARNING!** Using hardware power button, especially for events, can provide hardware light system freeze in rare situations! If lights are freezes, shutdown or hibernate you notebook (some lights can stay on after shutdown), disconnect power adapter and wait about 15 sec (or until lights turn off), then start it back.
- **WARNING!** There are well-known bug in DirectX at the Hybrid graphics (Intel+Nvidia) notebooks, which can prevent `alienfx-ambient` from capture screen. If you have only one screen (notebook panel) connected, but set Nvidia as a "Preferred GPU" in Nvidia panel, please add `alienfx-ambient` with "integrated GPU" setting at "Program settings" into the same panel. It will not work at default setting in this case.

## Installation
Download latest release archive from [here](https://github.com/T-Troll/alienfx-tools/releases).  
Unpack the archive to any directory of your choise.  
After unpack, run `alienfx-probe` or `alienfx-gui` to check and set light names (`alienfx-ambient` and `alienfx-haptics` will not work correctly wihout this operation).  
Run any tool you need from this folder!

## alienfx-probe Usage
`alienfx-probe.exe` is a probe for light IDs of the low-level DSK, and it assign names for them (similar to alienfx-led-tester, but wider device support) as well.  
Then run, it shows some info, then switch lights to green one-by-one and prompt to enter devices and lights name (you can enter name or ID from high-level SDK - it's also shown as a part of info). Then the name is set, light switched to blue. If you didn't see which light is changed, just press ENTER to skip it.  
It's check 16 first lights into the system by default, but you can change this value runnning `alienfx-probe.exe [number of lights]`.  
The purpose of this app is to check low-level API and to prepare light names for other apps, this names are stored and will be used in `alienfx-haptics` and `alienfx-ambient` as a light names for UI.

## alienfx-cli Usage
Run `alienfx-cli.exe` with a command and any options for that command. `alienfx-cli` uses low-level (USB driver) access by default, but you can switch to high-level SDK (Alienware LightFX) issuing `high-level` command. 
```
alienfx-cli.exe [command=option,option,option] ... [command=option,option,option] [loop]
```
The following commands are available:
- `status` Showing AlienFX device IDs and their lights IDs and status. Output is different for low- and high- level SDKs.
- `set-all=r,g,b[,br]` Sets all AlienFX lights to the specified color. Ex: `set-all=255,0,0` for red lights, `set-all=255,0,0,128` for dimmed red. NB: For low-level, it requires lights setup using `alienfx-probe`/-gui to work correctly!
- `set-one=<dev-id>,<light-id>,r,g,b[,br]` Set one light to color provided. Check light IDs using `status` command first. Ex: `set-one=0,1,0,0,255` - set light #2 at the device #1 to blue color. For low-level SDK, current active device will be used if devID=0, otherwise it switch to device with PID provided.
- `set-zone=<zone>,r,g,b[,br]` Set zone (see possible zones list below) light to color provided. This command only works with high-level API.
- `set-action=<dev-id>,<light-id>,<action>,r,g,b[,br[,<action>,r,g,b,br]]` Set light to color provided and enable action. You can define up to 9 actions in this command, but only first 1 or 2 will be used for high-level API and for older devices. For low-level SDK, current active device will be used if devID=0, otherwise it switch to device with PID provided.
- `set-zone-action=<action>,<zone>,r,g,b[,br,r,g,b[,br]]` Set zone light to color provided and enable action. This command only works with high-level API.
- `set-power=<light-id>,r,g,b,r,g,b` Set light as a hardware power button. First color for AC, 2nd for battery power. This command only works with low-level API.
- `set-tempo=<tempo>` Set next action tempo (in milliseconds).
- `set-dev=<pid>` Switch active device to this PID (low-level only).
- `Update` Updates light status (for looped commands or old devices).
- `Reset` Reset current device.
- `low-level` Next commands pass trough low-level API (USB driver) instead of high-level.
- `high-level` Next commands pass trough high-level API (Alienware LightFX), if it's available.
- `loop` Special command to continue all command query endlessly, until user interrupt it. It's provide possibility to keep colors even if awcc reset it. Should be last command in chain.

Supported Zones: `left, right, top, bottom, front, rear`  
Supported Actions: `pulse, morph (you need 2 colors for morph), color (disable action)`. For low-level api V3, it also support `breath, spectrum, rainbow`.

## alienfx-haptics Usage
Run `alienfx-haptics.exe`. Set the colors for lights and it’s mapping to respond the frequency.  
Keep app running or minimize, then start player or game of choice.
```
How it works
```
This application get audio stream from default output or input device (you can select it at the top menu), then made a real-time spectrum analysis.  
After that, spectrum powers grouped into 20 groups using octave scale.  
For each light found into the system, you can define group(s) it should react, as well as color for Lowest Hi power level into frequency group. If more, then one group is selected, power will be calculated as a medium power level across of them.  
It's also possible to compress diapason if group always not so or so high powered - use low-level and high-level sliders. Low-level slider define minimum power to react (all below will be treated as zero), and Hi-level slider defines maximum level (all above will be treated as maximum).  
”Clear” button set all colors to black and sliders to default value.  
”Refresh” button rescan all lights into the system (it’s useful if you connect/disconnect new light device) and restart audio capture stream (in case you switch or remove audio device).  
”Remove” button remove all lights settings across all lights. Use with care!  
”Minimize” button (or top menu minimize) will hide application into the system tray. Left-click the tray icon to open it back, right-click it to close application.

## alienfx-ambient Usage
Run `alienfx-ambient.exe`. At first launch, set screen zones mapping to lights and parameters.  
Keep it running or minimize, then start video player or game of choice.
```
How it works
```
This application get shot of screen (primary or secondary), then divide it to several zones.  
For each zone, dominant color calculated (you can see it at the button in app interface).  
For each light found into the system, you can define zone(s) it should follow. If more, then one zone selected for light, it will try to blend zone colors into one.  
You can also select which screen to grab - primary or secondary, if you have more, then one. You can also press "Reset Devices and Capture" button to re-initialize screen capturing and lights list.  
"Quality" slider defines how many pixels will be skipped at analysis - working with full-screen image is very slow. Increasing this value decrease CPU load, but decrease dominant color extraction precision as well. Default value is 16, ok for i7 CPU, you can decrease it if lights updates are slow (or CPU usage is so high), or increase if it works ok for you.  
"Dimming" slider decreases the overall lights brightness - use it for better fit you current monitor brightness.  
"Gamma Correction" checkbox enables visual color gamma correction, make them more close to screen one.  
”Restart devices and capture” button is used to refresh light list according to devices found into the systems, as well as restart screen capture process.  
”Minimize” button (or top menu minimize) will hide application into the system tray. Left-click the tray icon to open it back, right-click it to close application.  

## alienfx-gui Usage
Run `alienfx-gui.exe`.  
First, use "Devices and Lights" tab to configure out devices and lights settings, if you don't run `alienfx-probe` yet.  
Use "Color" tab for simple lights setup (this colors and modes will stay as default until AWCC run or modified by other app), even after reboot.  
You can also assign event for light to react on (power state, performance indicator, or just activity light), as well as a color for reaction at "Monitoring" tab.  
Tray menu (right-click on tray button) avaliable all the time for some fast switch functins, application hide to tray completely then minimized.  
```
How it works
```
`"Color"` tab is set hardware colors and effects for light. This setting will remains even if you exit application.  
Each light (except Power Button) can have up to 9 different hardware effects assigned to it, but some modes require more, then one effect (f.e. Morph – 2, Spectrum – 7) to work correctly.  
Use "Effects" list to add/remove/select effect. For each effect, you can define its color, effect mode, speed (how fast to change the color), and length (how long it plays).  
Available effect modes are:
- Color - Stay at solid color defined.
- Pulse - Switch between defined color and black.
- Morph - Morph light from previous color to current one. (for devices with APIv3 and higher)
- Breath - Morph light from black to current color. (for devices with APIv3 and higher)
- Spectrum - Like a morph, but up to 9 colors can be defined. (for devices with APIv3 and higher)
- Rainbow - Like a Color, but can use up to 9 colors. (for devices with APIv3 and higher)

Please keep in mind, mixing different event modes for one light can provide unexpected results, as well as last 2 modes can be unsupported for some lights (will do nothing). But you can experiment.  
“Set All” button copy current light effects to all lights into the list (it’s useful if you need to have all lights into the same color and mode).  

`"Monitoring"` tab designed for system events monitoring and change lights to reflect it - like power events, system load, temperatures.  
If "Use color settings as default" is active first color from "Color" tab will be used for "calm" situation, and the second color from "Monitoring" tab will be used for "active" situation, if it's not active - both colors will taken from "Monitoring" tab fields.  
Monitoring events available:  
System Load:
- CPU Load - CPU load color mix from 0% ("calm") to 100% ("Active")
- RAM Load - The same for used RAM percentage
- GPU Load - The same for utilized GPU percentage (top one across GPUs if more, then one present into the system).
- HDD Load - It's not exactly a load, but IDLE time. If idle - it's "calm", 100% busy - active, and mix between.
- Network load - Current network traffic value against maximal value detected (across all network adapters into the system).
- Max. Temperature - Maximal temperature in Celsius degree across all temperature sensors detected into the system.
- Battery level - Battery charge level in percent (100=discharged, 0=full).  
You can use "Minimal value" slider to define zone of no reaction - for example, for temperature it's nice to set it to the room temperature - only heat above it will change color.

Status Led:
- Disk activity - Switch light every disk activity event (HDD IDLE above zero).
- Network activity - Switch light if any network traffic detected (across all adapters).
- System overheat - Switch light if system temperature above cut level (default 95C, but you can change it using slider below).
- Out of memory - Switch light if memory usage above 90% (you can change it by the same slider).
- Low battery - Switch light if battery charged below the level defined by slider.  
"Blink" checkbox switch triggered value to blink between on-off colors 4 times per sec.

You can mix different monitoring type at once, f.e. different colors for same light for both CPU load and system overheat event. In this case Status color always override System one then triggered, as well as both override Power one.

`"Devices and lights"` tab is an extended GUI for `alienfx-probe`, providing devices and lights names and settings, name modification, light testing and some other hardware-related settings.  
"Devices" dropdown shows the list of the light devices found into the system, as well as selected device status (ok/error), you can also edit their names here.  
"Reset" button refresh the devices list (useful after you disconnect/connect new device), as well as re-open each device in case it stuck.  
"Lights" list shows all lights defined for selected device. Use “Add”/”Remove” buttons to add new light or remove selected one.  
NB: If you want to add new light, type light ID into LightID box before pressing “Add” button. If this ID already present in list or absent, it will be changed to the first unused ID. Don't try to enter light name at this stage, it's always set to default for easy recognition, change it later for desired one.  
"Reset light" button keep the light into the list, but removes all settings for this light from all profiles, so it will be not changed anymore until you set it up again.  
"Power button" checkbox set selected light as a "Hardware Power Button". After this operation, it will react to power source state (ac/battery/charging/sleep etc) automatically, but this kind of light change from the other app is a dangerous operation, and can provide unpleasant effects or light system hang.  
Selected light changes it color to the one defined by "Test color" button, and fade to black then unselected.

`"Profiles"` tab control profile settings, like selecting default profile, per-profile monitoring control and automatic switch to this profile then the defined application run.  
Each profile can have settings and application for trigger it. The settings are:
- "Application" - Defines application executable for trigger profile switch if "Profile auto switch" enabled.
- "Default profile" - Default profile is the one used if "Profile auto switch" enabled, but running applications doesn't fits any other profile. There is can be only one Default profile, and it can't be deleted.
- "Disable monitoring" - Then profile activated, monitoring functions are disabled, despite of global setting.
- "Dim lights" - Then profile activated, all lights are dimmed to common amount.
- "Only then active" - If "Profile auto switch" enabled, and application defined in profile running, profile will only be selected if application window active (have focus).

`"Settings"` tab is for application/global lights settings control - states, behavior, dimming, as well as application settings:
- "Turn on lights" - Operate all lights into the system. It will be black if this option disabled (default - on).
- "Turn off/dim lights then screen off" - Dim/Fade to black lights then system screen dimmed/off (default - off).
- "Off power button too" - Hardware Power button light follows the system state. Power light will be always on if disabled (default - off).
- "Autorefresh" - All lights will be refreshed 6 times per second. It's useful if you have AWCC running, but still want to control lights (default - off).
- "Color Gamma correction" - Enables color correction to make them looks close to screen one. It keep original AWCC colors if disabled (default - on).
- "Dim lights" - Dim system lights brightness. It's useful for night/battery scenario (default - off).
- "Dim Power button" - Power button follows system dim state. Power button will always have full brightness if disabled (default - off).
- "Dim lights on battery" - Automatically dim lights if system running at battery power, decreasing energy usage (default - on).
- "Dimming power" - Amount of the brightness decrease then dimmed. Values can be from 0 to 255, default is 92.
- "Start with Windows" - Start application at Windows start. It will not work if application request run as admin level (see below) (default - off).
- "Start minimized" - Hide application window in system tray after start.
- "Enable monitoring" - Application start to monitor system metrics (CPU/GPU/RAM load, etc) and refresh lights according to it (default - on).
- "Profile auto switch" - Switch between profiles available automatically, according of applications start and finish. This also block manual profile selection (default - off).
- "Disable AWCC" - Application will check active Alienware Control Center service at the each start and will try to stop it (and start back upon exit). It will require "Run as administrator" privilege (default - off).
- "Esif temperature" - Read hardware device temperature counters. If disabled, only system-wide ones will be read. It's useful for some Dell and Alienware systems, but also provide a lot of component temperature readings. It will require "Run as administrator" privilege (default - off).

Keyboard shortcuts (any time):
- CTRL+SHIFT+F12 - enable/disable lights
- CTRL+SHIFT+F11 - dim/undim lights
- CTRL+SHIFT+F10 - enable/disable system state monitoring
- F18 (on Alienware keyboards it's mapped to Fn+AlienFX) - cycle light mode (on-dim-off)

Other shortcuts (only then application active):
- ALT+c - switch to "Colors" tab
- ALT+m - switch to "Monitoring" tab
- ALT+d - switch to "Devices and Lights" tab
- ALT+p - switch to "Profiles" tab
- ALT+s - switch to "Settings" tab
- ALT+r - refresh all lights
- ALT+? - about app

**WARNING:** All color effects stop working if you enable any Event monitoring. It’s a hardware bug – any light update operation restart all effects.

## Tools Used
* Visual Studio Community 2019

## License
MIT. You can use these tools for any non-commercial or commercial use, modify it any way - supposing you provide a link to this page from you product page and mention me as the one of authors.

## Credits
Functionality idea and code, new devices support, haptic and ambient algorithms by T-Troll.  
Low-level SDK based on Gurjot95's [AlienFX_SDK](https://github.com/Gurjot95/AlienFX-SDK).  
API code and cli app is based on Kalbert312's [alienfx-cli](https://github.com/kalbert312/alienfx-cli).  
Spectrum Analyzer UI is based on Tnbuig's [Spectrum-Analyzer-15.6.11](https://github.com/tnbuig/Spectrum-Analyzer-15.6.11).  
FFT subroutine utilizes [Kiss FFT](https://sourceforge.net/projects/kissfft/) library.  
DXGi Screen capture based on Bryal's [DXGCap](https://github.com/bryal/DXGCap) example.  
Dominant light extraction math based on [OpenCV](https://github.com/opencv/opencv) library.  
Special thanks to [PhSHu](https://github.com/PhSMu) for ideas, testing and artwork.
