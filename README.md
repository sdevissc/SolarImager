# SolarImager

A Linux desktop application for solar and planetary imaging, combining direct-SDK camera streaming with the Airylab SSM seeing monitor for seeing-triggered lucky imaging capture.

![Platform](https://img.shields.io/badge/platform-Linux-blue)
![Qt](https://img.shields.io/badge/Qt-5-green)
![License](https://img.shields.io/badge/license-MIT-lightgrey)

---

## Features

### Camera
- **Direct SDK streaming** — no INDI required, full frame rate
  - Basler cameras via Pylon SDK
  - ZWO ASI cameras via ASI SDK
  - Player One cameras via Player One SDK
- **ROI drag-selection** — click and drag on the preview to set camera ROI
- **False color palettes** — Original, Inverted, Rainbow, Red Hot, Cool
- **Saturation highlight** — marks saturated pixels in red (Original/Inverted palettes)
- **Black/white point sliders** — display stretch, does not affect recorded data
- **Zoom** — Fit to viewport, 25% to 400%
- **Histogram** — linear/log scale

### Recording
- **SER recording** — 8-bit and 16-bit, duration in frames or seconds
- **Snap Frame** — saves a single frame as a FITS file with INSTRUME/EXPTIME/DATE-OBS keywords
- **Built-in SER player** — review captures in a dedicated tab

### SSM seeing monitor
- Integration with the [Airylab SSM](http://www.airylab.fr/ssm-seeing-monitor/) via USB serial
- Live seeing value and input level display
- Color-coded seeing graph (green below threshold, red above) with adjustable time range
- Auto-trigger recording when seeing stays below threshold for N consecutive samples
- CSV logging of seeing data with UTC timestamps
- Raw serial data monitor

### General
- **Settings dialog** — telescope, camera, display, recording and SSM defaults
- **Profile system** — save and reload named settings profiles as JSON
- **Sampling calculator** — arcsec/pixel and Shannon-Nyquist factor, always visible in toolbar
- **Tabbed interface** — Camera | SSM | SER Player
- **Desktop launcher** — installs an app icon for the GNOME/KDE launcher

---

## Interface overview

A thin **global toolbar** sits above the three tabs, always visible, showing:
- Current SSM seeing value, threshold spinbox, consecutive-sample count, auto-trigger toggle
- Sampling result (arcsec/pixel) and Shannon-Nyquist S/N factor
- **⚙ Settings** button

---

### 📷 Camera tab

**Left panel — Acquisition controls**
- Camera brand selector (Basler / ZWO / Player One) and Connect/Disconnect button
- Exposure time spinbox and logarithmic slider
- Gain spinbox and slider
- Pixel format selector (populated from the connected camera's capabilities)
- Recording duration (frames or seconds), file name, save directory
- **▶ Record** / **■ Stop** button and **Snap** button (single FITS frame)
- Progress bar during recording

**Left panel — ROI / Offset**
- Offset X/Y and Width/Height spinboxes
- Click and drag directly on the preview image to draw a ROI — coordinates snap to multiples of 4 (offsets) and 8 (size) to satisfy all SDK constraints
- Clear ROI button to return to full sensor

**Left panel — Histogram**
- 256-bin intensity histogram, updated live from the raw frame
- Linear/log Y axis toggle
- Black and white point sliders (display only — recorded data is always raw)

**Global toolbar — Sampling results**
- Telescope and camera parameters (aperture, focal length, pixel size, wavelength) are defined once in the **Settings dialog**
- Computed results are always visible in the toolbar: arcsec/pixel and S/N factor (green ≥ 3, orange 2–3, red < 2)

**Preview area**
- Zoom: Fit to viewport, 25% to 400%
- False color palette: Original, Inverted, Rainbow, Red Hot, Cool
- "Highlight sat. pixels" checkbox — marks saturated pixels in red (Original and Inverted palettes only)
- Grey background to avoid confusion with black tones in the image

---

### 📈 SSM tab

- Serial port and baud rate selector, Connect/Disconnect button
- **Current seeing value** — color-coded green (below threshold) or red (above)
- **Input level** — photodiode illumination indicator; values below 0.5 disable the auto-trigger
- **Mean seeing** over the session
- **Seeing graph** — color-coded line (green below threshold, red above), with exact color change at threshold crossings; time range selector (1 min / 5 min / 10 min / All)
- **CSV logging** — timestamped log of all seeing samples to a file
- **Raw serial monitor** — optional display of the raw token stream from the SSM

The threshold, consecutive-sample count and auto-trigger checkbox are also accessible directly from the global toolbar so they remain reachable while on the Camera tab during an observing session.

---

### ▶ SER Player tab

- Open any SER file for review — defaults to the same directory as the current recording save path
- Frame-by-frame navigation and playback
- Displays SER header metadata (camera, dimensions, bit depth, frame count)

---

## Requirements

### Build tools
- CMake ≥ 3.16
- C++17 compiler (GCC ≥ 9)
- Qt5 (Core, Widgets, Gui, SerialPort)

### Camera SDKs
| Camera brand | SDK | Default library path |
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

## Installation

### Desktop launcher (recommended)

Run once from the build directory to register the app in your GNOME/KDE launcher:

```bash
cd build
../install-desktop.sh
```

This installs the icon and `.desktop` file to `~/.local/share/` (no root required). The app will appear when searching in the GNOME Activities overview. Right-click → **Add to Favorites** to pin it to the dock.

If you move the build directory, simply re-run the script.

### System-wide install (optional)

```bash
cd build
sudo cmake --install . --prefix /usr
```

---

## SDK headers

The ZWO and Player One SDK headers (`ASICamera2.h`, `PlayerOneCamera.h`) are included in `src/sdk/` and distributed by their respective manufacturers. They are subject to their own license terms — see [DISCLAIMER.md](DISCLAIMER.md).

---

## Project structure

```
solar-imager.svg              # Application icon
solar-imager.desktop          # Desktop entry (template)
install-desktop.sh            # User-local launcher installer
src/
├── sdk/                      # Third-party SDK headers
│   ├── ASICamera2.h
│   └── PlayerOneCamera.h
├── CameraInterface.h/.cpp    # Abstract camera base class
├── BaslerCamera.h/.cpp       # Basler Pylon implementation
├── ZwoCameraInterface.h/.cpp # ZWO ASI implementation
├── PlayerOneCameraInterface.h/.cpp  # Player One implementation
├── FrameGrabber.h/.cpp       # Grab loop + SER writer
├── MainWindow.h/.cpp         # Main window and UI
├── PreviewWidget.h/.cpp      # Preview with ROI drag-selection
├── HistogramWidget.h/.cpp    # Histogram widget
├── SerPlayerDialog.h/.cpp    # Built-in SER player (tab 3)
├── SSMReader.h/.cpp          # Airylab SSM serial reader
├── SeePlot.h/.cpp            # Seeing history graph
├── AppSettings.h/.cpp        # Settings struct + JSON serialisation
├── SettingsDialog.h/.cpp     # Settings and profile dialog
└── Theme.h                   # Qt palette and button styles
```

---

## Supported SER format

Recordings use the standard SER format (magic bytes: `LUCAM-RECORDER`), compatible with:
- [PIPP](https://sites.google.com/site/astropipp/)
- [AutoStakkert](https://www.autostakkert.com/)
- [AstroSurface](https://www.astrosurface.com/)
- [Registax](https://www.astronomie.be/registax/)

---

## Settings and profiles

Open the **⚙ Settings** button in the toolbar to configure:
- **Telescope & Camera** — diameter, focal length, pixel size, wavelength, default format and bit depth
- **Display & Recording** — default palette, zoom, save directory, default duration
- **SSM** — default threshold, consecutive sample count, baud rate

Settings can be saved as named profiles in JSON format (`~/.config/SolarImager/profiles/`) and reloaded at any time. The `Default` profile is loaded automatically on startup if it exists.

---

## SSM seeing monitor

The [Airylab SSM](http://www.airylab.fr/ssm-seeing-monitor/) (Stellar Scintillation Monitor) measures atmospheric seeing in real time using a photodiode. It connects via USB serial at 115200 baud. When the input level drops below 0.5 (insufficient light), the auto-trigger is automatically disabled to avoid false recordings.

---

## Disclaimer

This software is provided for **personal, non-commercial use only**, with no warranty of any kind. See [DISCLAIMER.md](DISCLAIMER.md) for full terms, third-party SDK acknowledgements, and license information.

---

## License

MIT License — see [LICENSE](LICENSE) for details.  
Camera SDK libraries and headers are subject to their respective manufacturer licenses.
