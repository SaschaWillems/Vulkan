call ndk-build
if %ERRORLEVEL% EQU 0 (
	mkdir "assets\shaders\base"
	xcopy "..\..\data\shaders\base\*.spv" "assets\shaders\base" /Y
	
	mkdir "assets\shaders\specializationconstants"
	xcopy "..\..\data\shaders\specializationconstants\*.spv" "assets\shaders\specializationconstants" /Y

	mkdir "assets\models"
	xcopy "..\..\data\models\color_teapot_spheres.dae" "assets\models" /Y

	mkdir "assets\textures"
	xcopy "..\..\data\textures\metalplate_nomips_rgba.ktx" "assets\textures" /Y

	mkdir "res\drawable"
	xcopy "..\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanSpecializationconstants.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)