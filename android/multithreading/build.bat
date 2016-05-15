cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\base"
	xcopy "..\..\data\shaders\base\*.spv" "assets\shaders\base" /Y
	

	mkdir "assets\shaders\multithreading"
	xcopy "..\..\data\shaders\multithreading\*.spv" "assets\shaders\multithreading" /Y

    mkdir "assets\models"
	xcopy "..\..\data\models\retroufo_red_lowpoly.dae" "assets\models" /Y 
	xcopy "..\..\data\models\sphere.obj" "assets\models" /Y 
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanMultithreading.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
