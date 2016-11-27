cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	cd..

	mkdir "assets\shaders\base"
	xcopy "..\..\data\shaders\base\*.spv" "assets\shaders\base" /Y
	
	mkdir "assets\shaders\computenbody"
	xcopy "..\..\data\shaders\computenbody\*.spv" "assets\shaders\computenbody" /Y

	mkdir "assets\textures"
	xcopy "..\..\data\textures\particle01_rgba.ktx" "assets\textures" /Y
	xcopy "..\..\data\textures\particle_gradient_rgba.ktx" "assets\textures" /Y

	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanComputenbody.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
