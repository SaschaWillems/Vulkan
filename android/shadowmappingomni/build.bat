cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\shadowmapomni"
	xcopy "..\..\data\shaders\shadowmapomni\*.spv" "assets\shaders\shadowmapomni" /Y

	mkdir "assets\models"
	xcopy "..\..\data\models\shadowscene_fire.dae" "assets\models" /Y 
	xcopy "..\..\data\models\cube.obj" "assets\models" /Y 
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanShadowmappingomni.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
