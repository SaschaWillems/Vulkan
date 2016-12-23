cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	cd..

	mkdir "assets\shaders\base"
	xcopy "..\..\data\shaders\base\*.spv" "assets\shaders\base" /Y
	
	mkdir "assets\shaders\dynamicuniformbuffer"
	xcopy "..\..\data\shaders\dynamicuniformbuffer\*.spv" "assets\shaders\dynamicuniformbuffer" /Y

	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanDynamicuniformbuffer.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
