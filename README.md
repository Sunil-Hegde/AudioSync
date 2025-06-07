# This project is under active development!!

# AudioSync

A distributed audio system that turns multiple devices into synchronized speakers over WiFi.

## Overview

AudioSync connects phones, laptops, and tablets on the same network to create a DIY surround sound system. The project was inspired by wanting to enhance movie watching experiences without specialized audio equipment.

## Features

- Connect multiple devices as synchronized speakers
- Stream audio from a host device to all connected clients
- Real-time playback synchronization

## Technical Stack

- Languages: C/C++
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

**Server:** `./build/sender`

**Client:** `./build/client`

## Next Steps

- Audio capture and streaming
- Synchronization mechanism
- Cross-platform client applications

## License

This project is licensed under the [MIT License](LICENSE) - see the LICENSE file for details.
