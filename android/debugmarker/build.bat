cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\base"
	xcopy "..\..\data\shaders\base\*.spv" "assets\shaders\base" /Y
	
	mkdir "assets\shaders\debugmarker"
	xcopy "..\..\data\shaders\debugmarker\*.spv" "assets\shaders\debugmarker" /Y

	mkdir "assets\models"
	xcopy "..\..\data\models\treasure_smooth.dae" "assets\models" /Y
	xcopy "..\..\data\models\treasure_glow.dae" "assets\models" /Y
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanDebugmarker.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
