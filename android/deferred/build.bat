cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\deferred"
	xcopy "..\..\data\shaders\deferred\*.spv" "assets\shaders\deferred" /Y

	mkdir "assets\models\armor"
	xcopy "..\..\data\models\armor\*.*" "assets\models\armor" /Y

	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanDeferred.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
