cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\computeparticles"
	xcopy "..\..\data\shaders\computeparticles\*.spv" "assets\shaders\computeparticles" /Y

	mkdir "assets\textures"
	xcopy "..\..\data\textures\particle01_rgba.ktx" "assets\textures" /Y
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanComputeparticles.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
