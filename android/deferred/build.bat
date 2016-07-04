cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\base"
	xcopy "..\..\data\shaders\base\*.spv" "assets\shaders\base" /Y	

	mkdir "assets\shaders\deferred"
	xcopy "..\..\data\shaders\deferred\*.spv" "assets\shaders\deferred" /Y

	mkdir "assets\models\armor"
	xcopy "..\..\data\models\armor\*.*" "assets\models\armor" /Y

	mkdir "assets\textures"
	xcopy "..\..\data\textures\pattern_35_bc3.ktx" "assets\textures" /Y  
	xcopy "..\..\data\textures\pattern_35_normalmap_bc3.ktx" "assets\textures" /Y   

	mkdir "assets\models"
	xcopy "..\..\data\models\plane.obj" "assets\models" /Y  

	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanDeferred.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
