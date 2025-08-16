## Dependencies

### Enable I2C

### Docker

### How to Compile

```
mkdir build
cmake -S . -B build/ -DCMAKE_TOOLCHAIN_FILE=rpi3-toolchain.cmake
cmake --build build/
```