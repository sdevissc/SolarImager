# Disclaimer & Acknowledgements

## Purpose

SolarImager is a personal project developed for individual use in solar and 
planetary imaging. It is shared freely in the hope that it may be useful to 
other amateur astronomers, but comes with **no warranty of any kind**, either 
expressed or implied, including but not limited to fitness for a particular 
purpose, reliability, or correctness of results.

Use this software at your own risk. The authors accept no responsibility for 
any damage to equipment, loss of data, or any other consequence arising from 
the use of this software.

## Third-party SDKs

This software interfaces with the following third-party SDKs, which are the 
property of their respective manufacturers and are subject to their own 
license terms:

| SDK | Manufacturer | License |
|-----|-------------|---------|
| Pylon SDK | Basler AG | [Basler Software License](https://www.baslerweb.com) |
| ASI Camera SDK (ASICamera2) | ZWO | [ZWO SDK License](https://astronomy-imaging-camera.com) |
| Player One Camera SDK | Player One Astronomy | [Player One License](https://player-one-astronomy.com) |
| cfitsio | NASA/HEASARC | Public domain / NASA open source |

The SDK headers (`ASICamera2.h`, `PlayerOneCamera.h`) are distributed by 
their respective manufacturers and are included in this repository under 
`src/sdk/` for convenience. They remain the property of their respective 
manufacturers and are subject to their own license terms. If you prefer, 
you can obtain them directly from the manufacturer and replace the copies 
in `src/sdk/`.

## Third-party libraries

| Library | License |
|---------|---------|
| Qt 5 | LGPL v3 |
| cfitsio | NASA open source |

## Original code

All source code in `src/` (excluding SDK headers in `src/sdk/`) was written 
specifically for this project. No code has been copied from other open-source 
projects. Algorithms used (SER file format, plate scale formula, 
Shannon-Nyquist sampling criterion) are based on publicly documented 
astronomical standards and formulae.

## SER file format

The SER file format used for video recording was defined by Grischa Hahn and 
is documented at:  
http://www.grischa-hahn.homepage.t-online.de/astro/ser/

## No commercial use

This software is intended for **personal, non-commercial use only**. 
Redistribution for commercial purposes is not permitted without explicit 
written consent from the author.
