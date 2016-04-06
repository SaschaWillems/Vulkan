cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\pushconstants"
	xcopy "..\..\data\shaders\pushconstants\*.spv" "assets\shaders\pushconstants" /Y

	mkdir "assets\models"
	xcopy "..\..\data\models\samplescene.obj" "assets\models" /Y 
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanPushconstants.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
