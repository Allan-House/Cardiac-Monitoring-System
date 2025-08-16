## Dependencies

### Enable I2C

```
sudo i2cdetect -y 1
```

### Docker

### How to Compile

```
mkdir build
cmake -S . -B build/ -DCMAKE_TOOLCHAIN_FILE=rpi3-toolchain.cmake
cmake --build build/
```
