# Cardiac Monitoring System

A real-time embedded ECG (electrocardiogram) acquisition and analysis system for Raspberry Pi 3.

## Overview

This system performs continuous real-time ECG signal acquisition, processing, and analysis. It identifies cardiac waveform features (P-QRS-T complexes), stores data in multiple formats, and provides remote access via TCP.

### Key Features

- **Real-time ECG acquisition** at configurable sample rates.
- **Automatic waveform detection** - identifies P, Q, R, S, and T waves.
- **Dual data output** - binary and CSV formats.
- **TCP file server** - remote data retrieval over network.
- **Cross-platform** - runs on Raspberry Pi 3 (hardware) or desktop (file playback).
- **Thread-safe architecture** - concurrent acquisition, processing, and storage.
- **Graceful shutdown** - preserves all data on SIGINT/SIGTERM.

## System Architecture

```
┌───────────────────────────────────────────────────────────────┐
│                    Cardiac Monitoring System                  │
├───────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────────┐      ┌──────────────┐     ┌──────────────┐  │
│  │   AD8232     │ ───▶│   ADS1115    │───▶│ Raspberry Pi │  │
│  │ (ECG sensor) │      │ (16-bit ADC) │     │      3       │  │
│  └──────────────┘      └──────────────┘     └──────┬───────┘  │
│                                                    │          │
│   ┌────────────────────────────────────────────────┘          │
│   │                                                           │
│   ▼                                                           │
│   Acquisition Thread ───▶ RingBuffer (Raw) ───┐              │
│                                                │              │
│                                                │              │
│                                                ▼              │
│                                           ECG Analyzer        │
│                                           (P-QRS-T detect)    │
│                                                │              │
│                                                ▼              │
│                                     RingBuffer (Classified)   │
│                                                │              │
│                                                ▼              │
│                                          File Manager         │
│                                       (Binary + CSV writer)   │
│                                                │              │
│                                                ▼              │
│                                     data/processed/*.{bin,csv}│
│                                                │              │
│                                                ▼              │
│                                          TCP File Server      │
│                                           (Port 8080)         │
│                                                │              │
│                                                ▼              │
│                                       Remote Client (Python)  │
│                                                               │
└───────────────────────────────────────────────────────────────┘
```

## Hardware Requirements

### Components

- **Raspberry Pi 3 Model B/B+** (ARMv8 Cortex-A53).
- **ADS1115** - 16-bit I2C ADC (Texas Instruments).
- **AD8232** - Single-lead ECG front-end module.
- ECG electrodes (3-lead configuration).

### Connections

```
AD8232 ──▶ ADS1115 ──▶ Raspberry Pi 3
(Analog)   (I2C: A0)   (I2C: SDA/SCL)

AD8232 Connections:
  - LO+ → Digital monitoring (optional)
  - LO- → Digital monitoring (optional)
  - OUTPUT → ADS1115 A0
  - GND → Common ground
  - 3.3V → Power

ADS1115 Connections:
  - VDD → RPi 3.3V (Pin 1)
  - GND → RPi GND (Pin 6)
  - SCL → RPi SCL (Pin 5)
  - SDA → RPi SDA (Pin 3)
  - ADDR → GND (I2C address 0x48)
  - A0 → AD8232 OUTPUT
```

## Software Requirements

### Development Environment

- **CMake** 3.16+.
- **GCC/G++** with C++17 support.
- **WiringPi** library (for Raspberry Pi I2C).
- **Python 3** with numpy, matplotlib (for visualization).

### Cross-Compilation Toolchain

```bash
# Ubuntu/Debian
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
```

## Building the Project

### Native Build (Desktop - File Playback Mode)

Use this mode to test with pre-recorded ECG data files on your development machine.

```bash
cmake -S . -B build/
cmake --build build/
```

**Binary location:** `build/bin/Cardiac-Monitoring-System`

### Cross-Compilation (Raspberry Pi 3)

Build for the target hardware from your development machine.

```bash
cmake -S . -B build/ -DCMAKE_TOOLCHAIN_FILE=rpi3-toolchain.cmake
cmake --build build/
```

Transfer the binary to Raspberry Pi:

```bash
scp build-rpi3/bin/Cardiac-Monitoring-System pi@<rpi-ip>:~/
```

### Using DevContainer (Recommended)

The project includes a complete development environment:

1. Open project in VS Code.
2. Install "Dev Containers" extension.
3. Click "Reopen in Container".
4. Build as usual.

The container includes all required tools for both native and cross-compilation.

## Command-Line Options

```
Usage: Cardiac-Monitoring-System [OPTIONS]

OPTIONS:
  <filename>              Input ECG data file (file mode only)
  -d, --duration <sec>    Acquisition duration in seconds (default: 60)
  -h, --help              Show help message
```

## Output Files

Data is automatically saved in `data/processed/` with timestamps:

```
data/processed/
├── cardiac_data_20250112_143052.bin  # Binary format (compact)
└── cardiac_data_20250112_143052.csv  # CSV format (with classifications)
```

### File Formats

#### Binary Format (.bin)

Each sample: 10 bytes (little-endian)

```
[int16: voltage_raw] [int64: timestamp_us]
   (2 bytes)              (8 bytes)
```

Voltage conversion: `voltage = (voltage_raw * voltage_range) / 32768.0`

#### CSV Format (.csv)

```csv
timestamp_us,voltage,classification
0,0.524,N
4000,0.531,N
8000,0.538,N
12000,0.547,P
...
```

Classifications: `P`, `Q`, `R`, `S`, `T`, `N` (normal).

## Remote Data Access (TCP)

The system includes a TCP file server for remote data retrieval (hardware mode only).

### Server (Raspberry Pi)

Automatically starts on port 8080 when running on hardware. Sends files when a client connects.

### Client (Any Machine)

Use the included Python client:

```bash
# Basic usage
python3 tcp_client_python.py 192.168.1.100

# Custom port
python3 tcp_client_python.py 192.168.1.100 -p 8080

# Custom output directory
python3 tcp_client_python.py 192.168.1.100 -o my_data
```

Files are automatically received and saved to `received_data/` (or custom directory).

## Data Visualization

```bash
python3 ecg_plotter.py data/processed/cardiac_data_20250112_143052.csv
```

Features:
- Plots voltage vs time.
- Highlights detected waves (P, Q, R, S, T) with color-coded markers.
- Works with both binary and CSV files.

To run the data plotter (`ecg_plotter.py`) inside the development container, you need to run the following command on your host machine (outside the container):

```bash
xhost +local:docker
```

This command grants Docker containers permission to connect to your local X server, enabling Matplotlib (and other graphical applications) to display windows properly.

## Configuration

Edit `include/config.h` to modify system parameters:

## Logging

Logs are written to `cardiac_system.log` with color-coded console output:

- **CRITICAL** - System-critical errors.
- **ERROR** - Operation failures.
- **WARN** - Non-fatal issues.
- **SUCCESS** - Successful operations.
- **INFO** - General information.
- **DEBUG** - Detailed debugging (disabled in release builds).

## Troubleshooting

### I2C Issues (Raspberry Pi)

Enable I2C interface:
```bash
sudo raspi-config
# Interface Options → I2C → Enable
sudo reboot
```

Verify device detection:
```bash
sudo i2cdetect -y 1
# Should show device at 0x48
```

### Permission Errors

```bash
# Add user to i2c group
sudo usermod -a -G i2c $USER
# Log out and back in
```

### Build Errors

```bash
# Missing WiringPi (cross-compilation)
# Ensure libs/wiring-pi/wiringpi.a exists in project

# Missing Python dependencies
pip3 install numpy matplotlib scipy
```

## Development

### Code Structure

```
include/          # Header files
src/              # Implementation files
libs/wiring-pi/   # WiringPi library for RPi
data/
  ├── raw/        # Input data files (playback mode)
  └── processed/  # Output data files
.devcontainer/    # Development container config
```

### Key Components

- **Application** - Main coordinator, lifecycle management
- **DataSource** - Abstract interface (SensorData/FileData implementations)
- **ECGAnalyzer** - Real-time P-QRS-T wave detection
- **FileManager** - Dual-format file writing (binary + CSV)
- **TCPFileServer** - Network file transfer
- **RingBuffer** - Thread-safe circular buffer
- **Logger** - Multi-level logging system
- **SignalHandler** - Graceful shutdown on signals

### Building for Release

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

Release builds disable debug logging for better performance.

## License

This project is provided as-is for educational and research purposes.

## Author

Developed as an embedded ECG monitoring system with real-time analysis capabilities.