call conan install . --output-folder=build --profile=profile_debug --profile:b=profile_debug --build=missing
call cd build
call cmake ..  -G "Visual Studio 17 2022" -DCMAKE_TOOLCHAIN_FILE=build\conan_toolchain.cmake --fresh
call cd ..
