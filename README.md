# This project is under active development!!

# AudioSync

A distributed audio system that turns multiple devices into synchronized speakers over WiFi.

## Overview

AudioSync connects phones, laptops, and tablets on the same network to create a DIY surround sound system. The project was inspired by wanting to enhance movie watching experiences without specialized audio equipment. Currently, this project only supports systems with `Linux` and `macOS`.

## Current Features

- Connect a single device to send audio
- Stream audio from a host device to connected client
- Real-time playback synchronization
- Converted mp3 to raw PCM audio which is further broken down to audio packets for transmission
- Implemented custom Real-time Transmission Protocol(RTP)
- Uses PortAudio for real-time audio processing and playback

## Technical Stack

- Languages: C
- Audio: FFmpeg, PortAudio
- Networking: UDP/TCP for real-time streaming

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

### Build:

`make all`

### Run:

**Server:** `./build/sender input.raw`

**Client:** `./build/client <sender-ip-address>`

On Unix/Linux based systems, use `ip addr show` to find out sender's ip address.

## Next Steps

- Streaming to multiple devices
- Synchronization 
- Multicasting
- GUI Application

## License

This project is licensed under the [MIT License](LICENSE) - see the LICENSE file for details.
