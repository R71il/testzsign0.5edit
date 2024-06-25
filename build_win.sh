mkdir build; cd build
x86_64-w64-mingw32-g++ *.cpp SourceFiles/*.cpp -o zsign.exe -I../mman-win32 -I../openssl/include/ -L../openssl -L../mman-win32 -lmman -lcrypto -lgdi32 -std=c++11 -DWINDOWS -m64 -static-libgcc  -static-libstdc++
echo Build files have been written to ./build/zsign.exe