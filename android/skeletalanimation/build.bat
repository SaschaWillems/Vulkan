cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\base"
	xcopy "..\..\data\shaders\base\*.spv" "assets\shaders\base" /Y
	

	mkdir "assets\shaders\skeletalanimation"
	xcopy "..\..\data\shaders\skeletalanimation\*.spv" "assets\shaders\skeletalanimation" /Y
	mkdir "assets\shaders\base"
	xcopy "..\..\data\shaders\base\*.spv" "assets\shaders\base" /Y

	mkdir "assets\models"
	xcopy "..\..\data\models\goblin.dae" "assets\models" /Y
	xcopy "..\..\data\models\plane_z.obj" "assets\models" /Y

	mkdir "assets\textures"
	xcopy "..\..\data\textures\pattern_35_bc3.ktx" "assets\textures" /Y
	xcopy "..\..\data\textures\goblin_bc3.ktx" "assets\textures" /Y

	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanSkeletalanimation.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
