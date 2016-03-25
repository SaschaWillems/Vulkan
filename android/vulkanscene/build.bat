cd jni
call ndk-build
if %ERRORLEVEL% EQU 0 (
	echo ndk-build has failed, build cancelled
	cd..

	mkdir "assets\shaders\vulkanscene"
	xcopy "..\..\data\shaders\vulkanscene\*.spv" "assets\shaders\vulkanscene" /Y

	mkdir "assets\textures"
	xcopy "..\..\data\textures\cubemap_vulkan.ktx" "assets\textures" /Y

	mkdir "assets\models"
    xcopy "..\..\data\models\vulkanscenelogos.dae" "assets\models" /Y  
	xcopy "..\..\data\models\vulkanscenebackground.dae" "assets\models" /Y  
	xcopy "..\..\data\models\vulkanscenemodels.dae" "assets\models" /Y  
	xcopy "..\..\data\models\cube.obj" "assets\models" /Y  
	
	mkdir "res\drawable"
	xcopy "..\..\android\images\icon.png" "res\drawable" /Y

	call ant debug -Dout.final.file=vulkanVulkanscene.apk
) ELSE (
	echo error : ndk-build failed with errors!
	cd..
)
