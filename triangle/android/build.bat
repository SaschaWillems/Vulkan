cd jni
call ndk-build
if %ERRORLEVEL% GEQ 1 (
	echo ndk-build has failed, build cancelled
	cd..
	exit \b %errorlevel%
)
cd..

mkdir "assets\shaders"
xcopy "..\..\data\shaders\triangle.vert.spv" "assets\shaders" /Y
xcopy "..\..\data\shaders\triangle.frag.spv" "assets\shaders" /Y

call ant debug -Dout.final.file=vulkanTriangle.apk

if "%1" == "-deploy" (
	adb install -r vulkanTriangle.apk
)