cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders"
	xcopy "..\..\data\shaders\texture.vert.spv" "assets\shaders" /Y
	xcopy "..\..\data\shaders\texture.frag.spv" "assets\shaders" /Y

	mkdir "assets\textures"
	xcopy "..\..\data\textures\igor_and_pal_bc3.ktx" "assets\textures" /Y
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanTexture.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
