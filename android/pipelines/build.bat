cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\pipelines"
	xcopy "..\..\data\shaders\pipelines\*.spv" "assets\shaders\pipelines" /Y

	mkdir "assets\textures"
	xcopy "..\..\data\textures\crate_bc3.ktx" "assets\textures" /Y
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanPipelines.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
