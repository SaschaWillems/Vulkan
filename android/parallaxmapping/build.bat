cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\parallax"
	xcopy "..\..\data\shaders\parallax\*.spv" "assets\shaders\parallax" /Y

	mkdir "assets\textures"
	xcopy "..\..\data\textures\rocks_color_bc3.dds" "assets\textures" /Y
	xcopy "..\..\data\textures\rocks_normal_height_rgba.dds" "assets\textures" /Y

	mkdir "assets\models"
	xcopy "..\..\data\models\plane_z.obj" "assets\models" /Y

	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanParallaxmapping.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
