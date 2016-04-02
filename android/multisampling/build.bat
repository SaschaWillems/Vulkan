cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\mesh"
	xcopy "..\..\data\shaders\mesh\*.spv" "assets\shaders\mesh" /Y

	mkdir "assets\models\voyager"
	xcopy "..\..\data\models\voyager\*.*" "assets\models\voyager" /Y
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanMultisampling.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
