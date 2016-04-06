cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\bloom"
	xcopy "..\..\data\shaders\bloom\*.spv" "assets\shaders\bloom" /Y

	mkdir "assets\textures"
	xcopy "..\..\data\textures\cubemap_space.ktx" "assets\textures" /Y

	mkdir "assets\models"
	xcopy "..\..\data\models\retroufo.dae" "assets\models" /Y 
	xcopy "..\..\data\models\retroufo_glow.dae" "assets\models" /Y 
	xcopy "..\..\data\models\cube.obj" "assets\models" /Y 
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanBloom.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
