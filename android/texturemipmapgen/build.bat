cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	cd..

	mkdir "assets\shaders\base"
	xcopy "..\..\data\shaders\base\*.spv" "assets\shaders\base" /Y
	
	mkdir "assets\shaders\texturemipmapgen"
	xcopy "..\..\data\shaders\texturemipmapgen\*.spv" "assets\shaders\texturemipmapgen" /Y

	mkdir "assets\textures"
	xcopy "..\..\data\textures\metalplate_nomips_rgba.ktx" "assets\textures" /Y

	mkdir "assets\models"
	xcopy "..\..\data\models\tunnel_cylinder.dae" "assets\models" /Y
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanTexturemipmapgen.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
