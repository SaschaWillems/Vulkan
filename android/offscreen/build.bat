cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\offscreen"
	xcopy "..\..\data\shaders\offscreen\*.spv" "assets\shaders\offscreen" /Y

	mkdir "assets\textures"
	xcopy "..\..\data\textures\darkmetal_bc3.ktx" "assets\textures" /Y

	mkdir "assets\models"
	xcopy "..\..\data\models\plane.obj" "assets\models" /Y 
	xcopy "..\..\data\models\chinesedragon.dae" "assets\models" /Y 
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanOffscreen.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)