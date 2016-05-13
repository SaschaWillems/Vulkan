cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\instancing"
	xcopy "..\..\data\shaders\instancing\*.spv" "assets\shaders\instancing" /Y

	mkdir "assets\textures"
	xcopy "..\..\data\textures\texturearray_rocks_bc3.ktx" "assets\textures" /Y 
	
	mkdir "assets\models"
	xcopy "..\..\data\models\rock01.dae" "assets\models" /Y 
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanInstancing.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
