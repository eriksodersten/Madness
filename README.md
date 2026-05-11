# PWM Madness

Minimal JUCE VST3/AU/Standalone synth for slow PWM bass and drone sounds.

## Deploy for Ableton Live

```sh
/Users/eriksode/Projects/Madness/deploy.sh
```

This builds universal AU and VST3 bundles and copies them to:

```text
~/Library/Audio/Plug-Ins/Components/PWM Madness.component
~/Library/Audio/Plug-Ins/VST3/PWM Madness.vst3
```

## Build VST3 Only

```sh
cmake -S /Users/eriksode/Projects/Madness -B /Users/eriksode/Projects/Madness/build-current
cmake --build /Users/eriksode/Projects/Madness/build-current --target PWMMadness_VST3
```

Built VST3:

```text
/Users/eriksode/Projects/Madness/build-current/PWMMadness_artefacts/VST3/PWM Madness.vst3
```

## V1 Architecture

```text
OSC 1 Pulse
OSC 2 Pulse
SUB Sine
-> Mixer
-> tanh Drive
-> Lowpass Filter
-> Attack/Release VCA
-> DC Block
-> Soft Clip / Output Gain
```

Slow LFO modulation drives OSC 1 pulse width and inverted OSC 2 pulse width. PWM is clamped to avoid extreme duty cycles and is intentionally sub-audio.

## Parameters

- `mode`: MIDI / Always On
- `tune`
- `octave`
- `attack`
- `release`
- `osc1PW`
- `osc2PW`
- `osc2Detune`
- `oscMix`
- `subLevel`
- `pwmDepth`
- `pwmRate`
- `filterCutoff`
- `filterResonance`
- `drive`
- `outputGain`
- `madness`

## UI Asset

The project expects the final master panel at:

```text
Source/Assets/vintage_synth_madness_and_beach_vibes.png
```

That PNG should be the version without baked-in knob indicator lines. The JUCE editor draws knob pointers dynamically over transparent control hitboxes.
