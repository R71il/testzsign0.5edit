# zsign
Modified version of zsign, originally created by zhlynn.

## How to build

## Windows 
(This will compile a binary for windows, but you still have to compile it on Linux.)

### Prep work 

#### Install MingW64, make, and git
```bash
sudo apt-get install mingw-w64 git make
```

#### Download the required files
```bash
git clone https://github.com/openssl/openssl.git
git clone https://github.com/witwall/mman-win32.git
git clone https://github.com/tronkko/dirent
git clone https://github.com/selenic-io/zsign
```

#### Compile mman-win32

```bash
# From the root working dir (Includes the zsign/, openssl/, etc directories.)
cd mman-win32
./configure --cross-prefix=x86_64-w64-mingw32-
make
```

#### Compile openssl

```bash
# From the root working dir (Includes the zsign/, openssl/, etc directories.)
cd openssl # If you're currently in the mman-win32 remember to run cd ../ first :)
git checkout OpenSSL_1_0_2s
./Configure --cross-compile-prefix=x86_64-w64-mingw32- mingw64
make
```

#### Modify common.h

in `common/common.h` uncomment line 7 and replace it with `#include <PATH TO YOUR mman-win32/mman.h>`

#### Modify openssl.cpp

in `openssl.cpp` comment out line 8, and line 698.

#### Including dirent.h
```bash
# From the root working dir (Includes the zsign/, openssl/, etc directories.)
cd dirent/include # If you're currently in the mman-win32 remember to run cd ../ first :)
sudo cp dirent.h /usr/share/mingw-w64/include
```

### Building

```bash
# From inside the zsign/ directory
sudo chmod +x ./build.sh
sudo ./build.sh --win
```

## Linux

Fetch the repo, and cd into the directory.

in `common/common.h` uncomment line 8 (Make sure line 7 is still commented out.)

```bash
chmod +x ./build.sh
sudo ./build.sh
```

## MacOS

Fetch the repo, and cd into the directory.

in `common/common.h` uncomment line 8 (Make sure line 7 is still commented out.)

and then:
```bash
chmod +x ./build.sh
sudo ./build.sh
```

If you are running macOS on arm64 (Apple M-Series CPUs) you can compile a x86_64 binary for macOS by using the `--arch` cli flag. It accepts `arm64` & `x86_64`

```bash
sudo ./build.sh --arch x86_64
```