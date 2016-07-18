cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\base"
	xcopy "..\..\data\shaders\base\*.spv" "assets\shaders\base" /Y	

	mkdir "assets\shaders\deferredshadows"
	xcopy "..\..\data\shaders\deferredshadows\*.spv" "assets\shaders\deferredshadows" /Y

	mkdir "assets\models\armor"
	xcopy "..\..\data\models\armor\*.*" "assets\models\armor" /Y

	mkdir "assets\textures"
	xcopy "..\..\data\textures\pattern_57_diffuse_bc3.ktx" "assets\textures" /Y  
	xcopy "..\..\data\textures\pattern_57_normal_bc3.ktx" "assets\textures" /Y   

	mkdir "assets\models"
	xcopy "..\..\data\models\openbox.dae" "assets\models" /Y  

	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanDeferredshadows.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
