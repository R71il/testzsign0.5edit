MacOs=darwin
CentOs=yum
Ubuntu=apt-get
CmakeV=3.21.3

if [[ "$OSTYPE" =~ ^$MacOs ]]; then
    # Dependencies
    brew install zip unzip
    brew install openssl cmake
fi

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

if [ -x /usr/bin/$Ubuntu ]; then
    arch=$(uname -i)

    # Dependencies
    sudo apt-get install wget zip unzip build-essential checkinstall zlib1g-dev libssl-dev -y
    
    # Installing Cmake latest
    wget -qO- "https://cmake.org/files/v3.21/cmake-$CmakeV-linux-arm64.tar.gz" | \
    tar --strip-components=1 -xz -C /usr/local
fi

# Compile zsign using cmake
mkdir build; cd build
cmake ..
make