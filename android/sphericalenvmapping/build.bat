cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\sphericalenvmapping"
	xcopy "..\..\data\shaders\sphericalenvmapping\*.spv" "assets\shaders\sphericalenvmapping" /Y

	mkdir "assets\textures"
	xcopy "..\..\data\textures\matcap_array_rgba.ktx" "assets\textures" /Y

	mkdir "assets\models"
	xcopy "..\..\data\models\chinesedragon.dae" "assets\models" /Y 
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanSphericalenvmapping.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
