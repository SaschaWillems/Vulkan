cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\displacement"
	xcopy "..\..\data\shaders\displacement\*.spv" "assets\shaders\displacement" /Y

	mkdir "assets\textures"
	xcopy "..\..\data\textures\stonewall_colormap_bc3.dds" "assets\textures" /Y
	xcopy "..\..\data\textures\stonewall_heightmap_rgba.dds" "assets\textures" /Y


	mkdir "assets\models"
	xcopy "..\..\data\models\torus.obj" "assets\models" /Y 
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanDisplacement.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
