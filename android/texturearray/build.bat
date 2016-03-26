cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\texturearray"
	xcopy "..\..\data\shaders\texturearray\*.spv" "assets\shaders\texturearray" /Y

	mkdir "assets\textures"
	xcopy "..\..\data\textures\texturearray_bc3.ktx" "assets\textures" /Y

	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanTexturearray.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
