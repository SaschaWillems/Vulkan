cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\skeletalanimation"
	xcopy "..\..\data\shaders\skeletalanimation\*.spv" "assets\shaders\skeletalanimation" /Y

	mkdir "assets\models\astroboy"
	xcopy "..\..\data\models\astroboy\*.*" "assets\models\astroboy" /Y

	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanSkeletalanimation.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
