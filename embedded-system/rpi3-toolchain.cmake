# Toolchain file for Raspberry Pi 3 cross-compilation

# Set the system name and processor
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Set the compilers
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# Path to Raspberry Pi sysroot (optional, but recommended)
# set(CMAKE_SYSROOT /path/to/rpi/sysroot)

# Where to look for libraries and headers
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Raspberry Pi 3-specific flags
set(CMAKE_CXX_FLAGS_INIT "-mcpu=cortex-a53")
set(CMAKE_C_FLAGS_INIT "-mcpu=cortex-a53")