cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	cd..

	mkdir "assets\shaders\base"
	xcopy "..\..\data\shaders\base\*.spv" "assets\shaders\base" /Y
	
	mkdir "assets\shaders\terraintessellation"
	xcopy "..\..\data\shaders\terraintessellation\*.*" "assets\shaders\terraintessellation" /Y

	mkdir "assets\textures"
	xcopy "..\..\data\textures\terrain_heightmap_r16.ktx" "assets\textures" /Y
	xcopy "..\..\data\textures\terrain_texturearray_bc3.ktx" "assets\textures" /Y
	xcopy "..\..\data\textures\skysphere_bc3.ktx" "assets\textures" /Y

	mkdir "assets\models"
	xcopy "..\..\data\models\geosphere.obj" "assets\models" /Y
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanTerraintessellation.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
