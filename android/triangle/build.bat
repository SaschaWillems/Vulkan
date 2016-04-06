cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders"
	xcopy "..\..\data\shaders\triangle.vert.spv" "assets\shaders" /Y
	xcopy "..\..\data\shaders\triangle.frag.spv" "assets\shaders" /Y
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanTriangle.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
