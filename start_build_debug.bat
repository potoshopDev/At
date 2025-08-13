call cd build
call cmake --build . --target RedmineHelper
call cd bin\Debug\
call .\RedmineHelper.exe
call cd ..\..\..
