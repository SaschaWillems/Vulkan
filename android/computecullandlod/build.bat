cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	cd..

	mkdir "assets\shaders\base"
	xcopy "..\..\data\shaders\base\*.spv" "assets\shaders\base" /Y
	
	mkdir "assets\shaders\computecullandlod"
	xcopy "..\..\data\shaders\computecullandlod\*.spv" "assets\shaders\computecullandlod" /Y

	mkdir "assets\models"
	xcopy "..\..\data\models\suzanne_lods.dae" "assets\models" /Y  

	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanComputecullandlod.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
