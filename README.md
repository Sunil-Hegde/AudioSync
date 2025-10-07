# This project is under active development!!

# AudioSync

A distributed audio system that turns multiple devices into synchronized speakers over WiFi.

## Overview

AudioSync connects computers on the same network to create a DIY surround sound system. The project was inspired by wanting to enhance movie watching experiences without specialized audio equipment. Currently, this project only supports systems with `Linux` and `macOS`.

## Current Features

- Connect multiple devices to receive audio
- Stream audio from a host device to connected clients
- Uses UDP multicast for low-latency packet transfer
- Real-time playback synchronization using custom NTP-like protocol
- Uses PortAudio for real-time audio processing and playback
- Multiple users can join the stream mid-way and still stay in-sync.

## Technical Stack

- Languages: C
- Audio: FFmpeg, PortAudio
- Networking: UDP for real-time streaming, custom NTP-like protocol for syncronization

## Getting Started

### Prerequisites:

Before building, ensure you have PortAudio installed.
-   On Debian/Ubuntu-based systems:
    ```bash
    sudo apt-get update
    sudo apt-get install libportaudio2 libportaudio-dev
    ```
-   On macOS (using Homebrew):
    ```bash
    brew install portaudio
    ```
-   For other systems, please refer to the [PortAudio documentation](http://www.portaudio.com/docs/v19-doxydocs/tutorial_start.html).

### Audio File Requirements:

The sender currently expects PCM audio files with these specifications:
- **Format:** 16-bit PCM
- **Sample Rate:** 44100 Hz
- **Channels:** 2 (Stereo)

You can convert audio files using FFmpeg:
```bash
ffmpeg -i input.mp3 -f s16le -ar 44100 -ac 2 output.raw
```

### Build:

`make all`

### Run:

**Server:** `./build/sender input.raw`

**Client:** `./build/receiver`

**Note:** Start the receiver before starting the sender.

On Unix/Linux based systems, use `ip addr show` to find out sender's ip address.

**Important Notes:**
- Start the receiver before starting the sender
- All devices must be on the same network
- Uses multicast group `239.0.0.1:12345` by default
- On some systems, you may need to run with `sudo` for network permissions

## What this doesn't have and areas of improvement

- Currently uses a multicast setup, hence volume control or different audio for different devices is not supported. So, this cannot be called truly surrond sound system, but it does the job.
- The setup uses PortAudio for accessing the device's audio backend and implements only linux and macOS compilation in its makefile. Hence, only these two are supported as of now. While porting to windows is easy, building it for android will be significantly tougher since PortAudio doesn't support it officially.
- Currently, audio streaming is only possible from a file, work has to be started to send real-time audio data to facilitate actual speaker extension feature.
- GUI Application

## License

This project is licensed under the [MIT License](LICENSE) - see the LICENSE file for details.
