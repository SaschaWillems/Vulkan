cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\cubemap"
	xcopy "..\..\data\shaders\cubemap\*.spv" "assets\shaders\cubemap" /Y

	mkdir "assets\textures"
	xcopy "..\..\data\textures\cubemap_yokohama.ktx" "assets\textures" /Y

	mkdir "assets\models"
	xcopy "..\..\data\models\sphere.obj" "assets\models" /Y 
	xcopy "..\..\data\models\cube.obj" "assets\models" /Y 
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanTexturecubemap.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
