cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\particlefire"
	xcopy "..\..\data\shaders\particlefire\*.spv" "assets\shaders\particlefire" /Y

	mkdir "assets\textures"
	xcopy "..\..\data\textures\fireplace_colormap_bc3.ktx" "assets\textures" /Y
	xcopy "..\..\data\textures\fireplace_normalmap_bc3.ktx" "assets\textures" /Y
	xcopy "..\..\data\textures\particle_fire.ktx" "assets\textures" /Y
	xcopy "..\..\data\textures\particle_smoke.ktx" "assets\textures" /Y

	mkdir "assets\models"
	xcopy "..\..\data\models\fireplace.obj" "assets\models" /Y 
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanParticlefire.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
