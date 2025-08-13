call conan install . --output-folder=build --profile=profile_release --profile:b=profile_release
call cd build
call cmake ..  -G "Visual Studio 17 2022" -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake  -D__ADMIN_MODE=ON --fresh
call cmake --build . --target RedmineHelper --config Release
call cd ..
