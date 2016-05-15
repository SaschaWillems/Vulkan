cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\base"
	xcopy "..\..\data\shaders\base\*.spv" "assets\shaders\base" /Y
	

	mkdir "assets\shaders\pushconstants"
	xcopy "..\..\data\shaders\pushconstants\*.spv" "assets\shaders\pushconstants" /Y

	mkdir "assets\models"
	xcopy "..\..\data\models\samplescene.dae" "assets\models" /Y 
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanPushconstants.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
