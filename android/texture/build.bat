cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\base"
	xcopy "..\..\data\shaders\base\*.spv" "assets\shaders\base" /Y
	

	mkdir "assets\shaders\texture"
	xcopy "..\..\data\shaders\texture\*.spv" "assets\shaders\texture" /Y

	mkdir "assets\textures"
	xcopy "..\..\data\textures\pattern_02_bc2.ktx" "assets\textures" /Y
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanTexture.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
