cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\tessellation"
	xcopy "..\..\data\shaders\tessellation\*.spv" "assets\shaders\tessellation" /Y

	mkdir "assets\textures"
	xcopy "..\..\data\textures\bearmug.ktx" "assets\textures" /Y

	mkdir "assets\models\lowpoly"
	xcopy "..\..\data\models\lowpoly\bearmug.dae" "assets\models\lowpoly" /Y 
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanTessellation.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
