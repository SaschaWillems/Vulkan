cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	cd..

	mkdir "assets\shaders\base"
	xcopy "..\..\data\shaders\base\*.spv" "assets\shaders\base" /Y
	
	mkdir "assets\shaders\texture3d"
	xcopy "..\..\data\shaders\texture3d\*.spv" "assets\shaders\texture3d" /Y

	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanTexture3d.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
