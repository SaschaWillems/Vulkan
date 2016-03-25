cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\shadowmapping"
	xcopy "..\..\data\shaders\shadowmapping\*.spv" "assets\shaders\shadowmapping" /Y

	mkdir "assets\models"
	xcopy "..\..\data\models\vulkanscene_shadow.dae" "assets\models" /Y 
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanShadowmapping.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
