cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\raytracing"
	xcopy "..\..\data\shaders\raytracing\*.spv" "assets\shaders\raytracing" /Y

	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanRaytracing.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
