del cppbuild.exe

cmake -S src -B build
cmake --build build --config Release

copy build\Release\cppbuild.exe .