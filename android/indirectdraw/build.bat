cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\base"
	xcopy "..\..\data\shaders\base\*.spv" "assets\shaders\base" /Y
	
	mkdir "assets\shaders\indirectdraw"
	xcopy "..\..\data\shaders\indirectdraw\*.spv" "assets\shaders\indirectdraw" /Y

	mkdir "assets\textures"
	xcopy "..\..\data\textures\texturearray_plants_bc3.ktx" "assets\textures" /Y 
	xcopy "..\..\data\textures\ground_dry_bc3.ktx" "assets\textures" /Y 
	
	mkdir "assets\models"
	xcopy "..\..\data\models\plants.dae" "assets\models" /Y 
	xcopy "..\..\data\models\plane_circle.dae" "assets\models" /Y 
	xcopy "..\..\data\models\skysphere.dae" "assets\models" /Y 
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanIndirectdraw.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
