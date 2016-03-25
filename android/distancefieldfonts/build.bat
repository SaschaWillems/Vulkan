cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\distancefieldfonts"
	xcopy "..\..\data\shaders\distancefieldfonts\*.spv" "assets\shaders\distancefieldfonts" /Y

	mkdir "assets\textures"
	xcopy "..\..\data\textures\font_sdf_rgba.ktx" "assets\textures" /Y
	xcopy "..\..\data\textures\font_bitmap_rgba.ktx" "assets\textures" /Y

	xcopy "..\..\data\font.fnt" "assets" /Y
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanDistancefieldfonts.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
