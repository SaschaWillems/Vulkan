cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	cd..

	mkdir "assets\shaders\base"
	xcopy "..\..\data\shaders\base\*.spv" "assets\shaders\base" /Y

	mkdir "assets\shaders\ssao"
	xcopy "..\..\data\shaders\ssao\*.spv" "assets\shaders\ssao" /Y

	mkdir "assets\models\sibenik"
	xcopy "..\..\data\models\sibenik\sibenik.dae" "assets\models\sibenik" /Y  

	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanSSAO.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
