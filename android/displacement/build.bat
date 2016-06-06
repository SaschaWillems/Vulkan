cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\base"
	xcopy "..\..\data\shaders\base\*.spv" "assets\shaders\base" /Y
	

	mkdir "assets\shaders\displacement"
	xcopy "..\..\data\shaders\displacement\*.spv" "assets\shaders\displacement" /Y

	mkdir "assets\textures"
	xcopy "..\..\data\textures\pattern_36_bc3.ktx" "assets\textures" /Y

	mkdir "assets\models"
	xcopy "..\..\data\models\plane.obj" "assets\models" /Y 
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanDisplacement.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
