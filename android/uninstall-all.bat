@echo off
SET /P ANSWER=Uninstall all vulkan examples from attached device (Y/N)?
if /i {%ANSWER%}=={y} (goto :uninstall) 
if /i {%ANSWER%}=={yes} (goto :uninstall) 
goto :exit 

:uninstall
adb uninstall de.saschawillems.vulkanGeometryshader
adb uninstall de.saschawillems.vulkanComputeparticles
adb uninstall de.saschawillems.vulkanComputeshader
adb uninstall de.saschawillems.vulkanParallaxmapping
adb uninstall de.saschawillems.vulkanBloom
adb uninstall de.saschawillems.vulkanGears
adb uninstall de.saschawillems.vulkanTexturecubemap
adb uninstall de.saschawillems.vulkanInstancing
adb uninstall de.saschawillems.vulkanDeferred
adb uninstall de.saschawillems.vulkanParticlefire
adb uninstall de.saschawillems.vulkanOcclusionquery
adb uninstall de.saschawillems.vulkanTexture
adb uninstall de.saschawillems.vulkanTessellation
adb uninstall de.saschawillems.vulkanMesh
adb uninstall de.saschawillems.vulkanTexturearray
adb uninstall de.saschawillems.vulkanPipelines
adb uninstall de.saschawillems.vulkanTriangle
adb uninstall de.saschawillems.vulkanSkeletalanimation
adb uninstall de.saschawillems.vulkanDistancefieldfonts
adb uninstall de.saschawillems.vulkanVulkanscene
adb uninstall de.saschawillems.vulkanOffscreen
adb uninstall de.saschawillems.vulkanShadowmapping
adb uninstall de.saschawillems.vulkanPushconstants
adb uninstall de.saschawillems.vulkanShadowmappingomni
adb uninstall de.saschawillems.vulkanSphericalenvmapping
adb uninstall de.saschawillems.vulkanRadialblur
adb uninstall de.saschawillems.vulkanDisplacement
adb uninstall de.saschawillems.vulkanRaytracing
adb uninstall de.saschawillems.vulkanMultisampling
goto finish

:exit
echo Cancelled

:finish