cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\geometryshader"
	xcopy "..\..\data\shaders\geometryshader\*.spv" "assets\shaders\geometryshader" /Y
    
	mkdir "assets\models"
	xcopy "..\..\data\models\suzanne.obj" "assets\models" /Y 
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanGeometryshader.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
