# C++ development

## Install dependency libraries

```shell
cd external
sh install.sh
```

## Build

```shell
cd ../src
cmake -DCMAKE_BUILD_TYPE="Debug" -DCMAKE_PREFIX_PATH=`pwd`"/external" -H. -Bbuild
cd build && make -j4
```

## Launch

```shell
./bin/PICO -f ../config/config.yaml
```
