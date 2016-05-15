cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\base"
	xcopy "..\..\data\shaders\base\*.spv" "assets\shaders\base" /Y
	

	mkdir "assets\shaders\textoverlay"
	xcopy "..\..\data\shaders\textoverlay\*.spv" "assets\shaders\textoverlay" /Y

	mkdir "assets\models"
	xcopy "..\..\data\models\cube.dae" "assets\models" /Y

	mkdir "assets\textures"
	xcopy "..\..\data\textures\skysphere_bc3.ktx" "assets\textures" /Y
	xcopy "..\..\data\textures\round_window_bc3.ktx" "assets\textures" /Y
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanTextoverlay.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
