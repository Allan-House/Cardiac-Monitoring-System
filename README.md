## Dependencies

### Enable I2C

```
sudo i2cdetect -y 1
```

### Docker

```
xhost +local:docker
```

### How to Compile

```
mkdir build
cmake -S . -B build/ -DCMAKE_TOOLCHAIN_FILE=rpi3-toolchain.cmake -DCMAKE_BUILD_TYPE=Debug
cmake --build build/
```
