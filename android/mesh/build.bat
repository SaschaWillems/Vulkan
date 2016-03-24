cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\mesh"
	xcopy "..\..\data\shaders\mesh\mesh.vert.spv" "assets\shaders\mesh" /Y
	xcopy "..\..\data\shaders\mesh\mesh.frag.spv" "assets\shaders\mesh" /Y

	mkdir "assets\models\voyager"
	xcopy "..\..\data\models\voyager\voyager.ktx" "assets\models\voyager" /Y
	xcopy "..\..\data\models\voyager\voyager.obj" "assets\models\voyager" /Y
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanMesh.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
