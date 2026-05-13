# SGM Reverb

High-function algorithmic reverb plug-in built with JUCE/CMake.

## Features

- VST3 and standalone builds
- Predelay, size, decay, diffusion, density, damping, low cut, high cut
- Stereo width and wet/dry mix
- Modulated FDN tail
- Early reflections
- Ducking
- Freeze mode
- Shimmer octave-up feedback layer

## VST Overview

- Format: VST3 and standalone app
- Type: audio effect, not a synth or MIDI effect
- Channel layout: mono or stereo input/output
- Tail: reports a 12 second tail to the host
- State: all parameters are saved and restored by the host session

## Controls

- Mix: dry/wet balance.
- Predelay: time before the reverb tail begins.
- Size: perceived room size.
- Decay: tail length and persistence.
- Diffusion: density of early scattering.
- Density: density of the late reverb tail.
- Damping: high-frequency absorption inside the reverb.
- Low Cut: removes low-frequency buildup from the reverb.
- High Cut: darkens the reverb tail.
- Width: stereo width of the wet signal.
- Modulation: movement inside the reverb tail.
- Ducking: lowers the wet signal while the dry input is present.
- Shimmer: adds an octave-up feedback layer.
- Freeze: sustains the current reverb buffer.

## Build

Requirements:

- CMake 3.22 or later
- Visual Studio 2022 with the Desktop development with C++ workload
- Internet access on the first configure so CMake can fetch JUCE 7.0.12

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The local build output is:

```text
build/SGMReverb_artefacts/Release/VST3/SGM Reverb.vst3
build/SGMReverb_artefacts/Release/Standalone/SGM Reverb.exe
```

To install the plug-in for most Windows hosts, copy the `.vst3` bundle to:

```text
C:\Program Files\Common Files\VST3
```

After copying, rescan plug-ins in your DAW and look for `SGM Reverb`.

## Installer

This project includes a WiX v4 installer definition. Install the WiX CLI once:

```powershell
dotnet tool install --global wix
```

Then build the plug-in and create the MSI:

```powershell
cmake --build build --config Release
.\Installer\build-installer.ps1 -Configuration Release -ProductVersion 0.1.0 -AcceptEula
```

If WiX is available when CMake configures the project, you can also run:

```powershell
cmake --build build --config Release --target installer
```

WiX v7 requires EULA acceptance before building. You can either pass `-AcceptEula`
to the script or run `wix eula accept wix7` once before using the CMake
`installer` target.

The MSI is written to:

```text
build/installer/SGM-Reverb-0.1.0-win64.msi
```

The installer places the VST3 plug-in in:

```text
C:\Program Files\Common Files\VST3\SGM Reverb.vst3
```

It also installs the standalone app in:

```text
C:\Program Files\Sagami\SGM Reverb
```

The MSI uses the standard WiX install wizard. Its install directory page points
to the standard VST3 location.

## License
MIT License
