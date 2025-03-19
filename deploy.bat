@echo off

echo Copying code...
xcopy "mrklang\runtime_generated.cpp" "runtime\src\runtime_generated.cpp" /Y

echo Copying metadata..
xcopy "mrklang\runtime_metadata.mrkmeta" "runtime\runtime_metadata.mrkmeta" /Y

echo Done!
pause