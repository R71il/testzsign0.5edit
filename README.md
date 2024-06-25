# zsign
#### cross-platform codesigning utility to sign .ipa or .app bundles, can also inject dyllibs into apps :)
Modified version of zsign, originally created by zhlynn.

## How to build
Follow the given instructions to compile it for your target OS.

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

```bash
Usage: zsign [-options] [-k privkey.pem] [-m dev.prov] [-o output.ipa] file|folder

options:
-k, --pkey           Path to private key or p12 file. (PEM or DER format)
-m, --prov           Path to mobile provisioning profile.
-c, --cert           Path to certificate file. (PEM or DER format)
-d, --debug          Generate debug output files. (.zsign_debug folder)
-f, --force          Force sign without cache when signing folder.
-o, --output         Path to output ipa file.
-p, --password       Password for private key or p12 file.
-b, --bundle_id      New bundle id to change.
-n, --bundle_name    New bundle name to change.
-r, --bundle_version New bundle version to change.
-e, --entitlements   New entitlements to change.
-z, --zip_level      Compressed level when output the ipa file. (0-9)
-l, --dylib          Path to inject dylib file.
-w, --weak           Inject dylib as LC_LOAD_WEAK_DYLIB.
-i, --install        Install ipa file using ideviceinstaller command for test.
-q, --quiet          Quiet operation.
-v, --version        Show version.
-h, --help           Show help.
```

1. Show mach-o and codesignature segment info.
```bash
./zsign demo.app/execute
```

2. Sign ipa with private key and mobileprovisioning file.
```bash
./zsign -k privkey.pem -m dev.prov -o output.ipa -z 9 demo.ipa
```

3. Sign folder with p12 and mobileprovisioning file (using cache).
```bash
./zsign -k dev.p12 -p 123 -m dev.prov -o output.ipa demo.app
```

4. Sign folder with p12 and mobileprovisioning file (without cache).
```bash
./zsign -f -k dev.p12 -p 123 -m dev.prov -o output.ipa demo.app
```

5. Inject dylib into ipa and re-sign.
```bash
./zsign -k dev.p12 -p 123 -m dev.prov -o output.ipa -l demo.dylib demo.ipa
```

6. Change bundle id and bundle name
```bash
./zsign -k dev.p12 -p 123 -m dev.prov -o output.ipa -b 'com.tree.new.bee' -n 'TreeNewBee' demo.ipa
```

7. Inject dylib(LC_LOAD_DYLIB) into mach-o file.
```bash
./zsign -l "@executable_path/demo.dylib" demo.app/execute
```

8. Inject dylib(LC_LOAD_WEAK_DYLIB) into mach-o file.
```bash
./zsign -w -l "@executable_path/demo.dylib" demo.app/execute
```

## Credits

#### Original Codebase
* **zsign** @zhlynn https://github.com/zhlynn/zsign

#### PRs

* @camila314 https://github.com/zhlynn/zsign/pull/301
* @mforys https://github.com/zhlynn/zsign/pull/250