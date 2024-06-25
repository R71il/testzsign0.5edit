MacOs=darwin
CentOs=yum
Ubuntu=apt-get
CmakeV=3.21.3
arch=""
win_build=false

# Function to show usage
usage() {
    echo "Usage: $0 [--win] [--arch x64|arm64]"
    exit 1
}

# Parse arguments
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --win) win_build=true ;;
        --arch)
            if [[ "$2" == "x86_64" || "$2" == "arm64" ]]; then
                arch="$2"
                shift
            else
                echo "Invalid value for --arch. Must be 'x86_64' or 'arm64'."
                usage
            fi
            ;;
        *) echo "Unknown parameter: $1" ; usage ;;
    esac
    shift
done

# If no --arch specified, detect the architecture
if [[ -z "$arch" ]]; then
    arch=$(uname -m)
fi

# macOS
if [[ "$OSTYPE" =~ ^$MacOs ]]; then
    # Dependencies
    brew install zip unzip
    brew install openssl cmake
fi

# CentOS
if [ -x /usr/bin/$CentOs ]; then
    arch=$(uname -i)

    # Dependencies
    yum install openssl-devel -y;
    yum install wget zip unzip -y
    yum group install "Development Tools" -y
    
    # Installing Cmake latest
    wget -qO- "https://cmake.org/files/v3.21/cmake-$CmakeV-linux-$arch.tar.gz" | \
    tar --strip-components=1 -xz -C /usr/local
fi

# Ubuntu
if [ -x /usr/bin/$Ubuntu ]; then
    arch=$(uname -i)

    # Dependencies
    sudo apt-get install wget zip unzip build-essential checkinstall zlib1g-dev libssl-dev -y
    
    # Installing Cmake latest
    wget -qO- "https://cmake.org/files/v3.21/cmake-$CmakeV-linux-arm64.tar.gz" | \
    tar --strip-components=1 -xz -C /usr/local
fi

# Build
if $win_build; then
    echo Building for Windows.
    mkdir -p build
    x86_64-w64-mingw32-g++ *.cpp SourceFiles/*.cpp -o build/zsign.exe -I../mman-win32 -I../openssl/include/ -L../openssl -L../mman-win32 -lmman -lcrypto -lgdi32 -std=c++11 -DWINDOWS -m64 -static-libgcc -static-libstdc++
    echo "Build files have been written to ./build/zsign.exe"
else
        # Compile zsign using cmake
    if [[ "$OSTYPE" =~ ^$MacOs && "$arch" == "x86_64" ]]; then
        echo Building for macOS x86_64
        mkdir -p build && cd build
        cmake -DCMAKE_OSX_ARCHITECTURES=x86_64 ..
        make
    elif [[ "$OSTYPE" =~ ^$MacOs && "$arch" == "arm64" ]]; then
        echo Building for macOS arm64
        mkdir -p build
        g++ *.cpp SourceFiles/*.cpp -lcrypto -I/opt/homebrew/Cellar/openssl@3/3.2.1/include -L/opt/homebrew/Cellar/openssl@3/3.2.1/lib -O3 -o build/zsign
    else
        mkdir -p build && cd build
        echo Building for Linux $arch
        cmake ..
        make
    fi
fi