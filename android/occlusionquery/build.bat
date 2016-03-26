cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\occlusionquery"
	xcopy "..\..\data\shaders\occlusionquery\*.spv" "assets\shaders\occlusionquery" /Y

	mkdir "assets\models"
	xcopy "..\..\data\models\plane_z.3ds" "assets\models" /Y 
	xcopy "..\..\data\models\teapot.3ds" "assets\models" /Y 
	xcopy "..\..\data\models\sphere.3ds" "assets\models" /Y 
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanOcclusionquery.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
