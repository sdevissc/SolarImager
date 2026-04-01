# SolarImager

A Linux desktop application for solar and planetary imaging, combining direct-SDK camera streaming with the Airylab SSM seeing monitor for seeing-triggered lucky imaging capture.

![Platform](https://img.shields.io/badge/platform-Linux-blue)
![Qt](https://img.shields.io/badge/Qt-5-green)
![License](https://img.shields.io/badge/license-MIT-lightgrey)

---

## Features

- **Direct SDK camera streaming** — no INDI required, full frame rate
  - Basler cameras via Pylon SDK
  - ZWO ASI cameras via ASI SDK
  - Player One cameras via Player One SDK
- **SSM seeing monitor** integration (Airylab SSM, serial/USB)
  - Live seeing and input level display
  - Auto-trigger recording when seeing drops below threshold
  - CSV logging of seeing data with UTC timestamps
- **SER recording** — 8-bit and 16-bit, compatible with PIPP, AutoStakkert, AstroSurface
- **Snap Frame** — saves a single frame as a FITS file with keywords
- **Built-in SER player** — review captures without leaving the application
- **Preview controls** — zoom (Fit to 400%), false color palettes, black/white point sliders
- **Histogram** — linear/log scale with saturated and zero-level pixel percentages
- **ROI drag-selection** — click and drag on the preview to set camera ROI
- **Sampling calculator** — computes arcsec/pixel and Shannon-Nyquist factor from aperture, focal length, pixel size and wavelength

---

## Requirements

### Build tools
- CMake ≥ 3.16
- C++17 compiler (GCC ≥ 9)
- Qt5 (Core, Widgets, Gui, SerialPort)

### Camera SDKs
| Camera brand | SDK | Default path |
|---|---|---|
| Basler | [Pylon SDK](https://www.baslerweb.com/en/downloads/software-downloads/) | `/opt/pylon` |
| ZWO | [ASI SDK](https://astronomy-imaging-camera.com/software-drivers) | `/usr/local/lib` |
| Player One | [Player One SDK](https://player-one-astronomy.com/service/software.html) | `/usr/local/lib` |

### Other dependencies
| Library | Install |
|---|---|
| cfitsio | `sudo apt install libcfitsio-dev` |

---

## Building

```bash
# Install Qt5 and cfitsio
sudo apt install qtbase5-dev libqt5serialport5-dev libcfitsio-dev

# Install camera SDKs (see links above), then:
mkdir build && cd build
cmake .. -DPYLON_ROOT=/opt/pylon
make -j$(nproc)
```

### CMake options
| Option | Default | Description |
|---|---|---|
| `PYLON_ROOT` | `/opt/pylon` | Path to Basler Pylon SDK root |

---

## SDK headers

The ZWO and Player One SDK headers (`ASICamera2.h`, `PlayerOneCamera.h`) are
included in `src/sdk/` and are distributed by their respective manufacturers.
They are subject to their own license terms — see [DISCLAIMER.md](DISCLAIMER.md).

---

## Project structure

```
src/
├── sdk/                             # Third-party SDK headers
│   ├── ASICamera2.h
│   └── PlayerOneCamera.h
├── CameraInterface.h                # Abstract camera base class
├── BaslerCamera.h/.cpp              # Basler Pylon implementation
├── ZwoCameraInterface.h/.cpp        # ZWO ASI implementation
├── PlayerOneCameraInterface.h/.cpp  # Player One implementation
├── FrameGrabber.h/.cpp              # Grab loop + SER writer
├── MainWindow.h/.cpp                # Main UI
├── PreviewWidget.h/.cpp             # Preview with ROI drag-selection
├── HistogramWidget.h/.cpp           # Histogram display
├── SerPlayerDialog.h/.cpp           # Built-in SER file player
├── SSMReader.h/.cpp                 # Airylab SSM serial reader
├── SeePlot.h/.cpp                   # Seeing history plot
└── Theme.h                          # Qt palette / button styles
```

---

## Supported SER format

Recordings are standard SER files (magic: `LUCAM-RECORDER`) compatible with:
- [PIPP](https://sites.google.com/site/astropipp/)
- [AutoStakkert](https://www.autostakkert.com/)
- [AstroSurface](https://www.astrosurface.com/)
- [Registax](https://www.astronomie.be/registax/)

---

## SSM seeing monitor

The [Airylab SSM](http://www.airylab.fr/ssm-seeing-monitor/) connects via USB serial (115200 baud, 8N1). The protocol sends continuous ASCII tokens:

```
A<float>$   — seeing index in arcseconds
B<float>$   — photodiode input level (0.5–1.0 = valid signal)
```

Input level below 0.5 (no light on photodiode) disables the auto-trigger to avoid false triggers.

---

## Disclaimer

This software is provided for **personal, non-commercial use only**, with no warranty of any kind. See [DISCLAIMER.md](DISCLAIMER.md) for full terms, third-party SDK acknowledgements, and license information.

---

## License

MIT License — see [LICENSE](LICENSE) for details.  
Camera SDK libraries and headers are subject to their respective manufacturer licenses.
