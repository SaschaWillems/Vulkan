# Remove all examples from connected device(s)
import subprocess
import sys

APP_NAMES = [
    "de.saschawillems.vulkanBloom",
    "de.saschawillems.vulkanComputecullandlod",
    "de.saschawillems.vulkanComputenbody",
    "de.saschawillems.vulkanComputeparticles",
    "de.saschawillems.vulkanComputeshader",
    "de.saschawillems.vulkanDebugmarker",
    "de.saschawillems.vulkanDeferred",
    "de.saschawillems.vulkanDeferredmulitsampling",
    "de.saschawillems.vulkanDeferredshadows",
    "de.saschawillems.vulkanDisplacement",
    "de.saschawillems.vulkanDistancefieldfonts",
    "de.saschawillems.vulkanDynamicuniformbuffer",
    "de.saschawillems.vulkanGears",
    "de.saschawillems.vulkanGeometryshader",
    "de.saschawillems.vulkanHDR",
    "de.saschawillems.vulkanImGui",
    "de.saschawillems.vulkanIndirectdraw",
    "de.saschawillems.vulkanInstancing",
    "de.saschawillems.vulkanMesh",
    "de.saschawillems.vulkanMultisampling",
    "de.saschawillems.vulkanMultithreading",
    "de.saschawillems.vulkanOcclusionquery",
    "de.saschawillems.vulkanOffscreen",
    "de.saschawillems.vulkanPBRBasic",
    "de.saschawillems.vulkanPBRIBL",
    "de.saschawillems.vulkanParallaxmapping",
    "de.saschawillems.vulkanParticlefire",
    "de.saschawillems.vulkanPipelines",
    "de.saschawillems.vulkanPushconstants",
    "de.saschawillems.vulkanRadialblur",
    "de.saschawillems.vulkanRaytracing",
    "de.saschawillems.vulkanSSAO",
    "de.saschawillems.vulkanScenerendering",
    "de.saschawillems.vulkanShadowmapping",
    "de.saschawillems.vulkanShadowmappingomni",
    "de.saschawillems.vulkanSkeletalanimation",
    "de.saschawillems.vulkanSpecializationconstants",
    "de.saschawillems.vulkanSphericalenvmapping",
    "de.saschawillems.vulkanSubpasses",
    "de.saschawillems.vulkanTerraintessellation",
    "de.saschawillems.vulkanTessellation",
    "de.saschawillems.vulkanTextoverlay",
    "de.saschawillems.vulkanTexture",
    "de.saschawillems.vulkanTexture3d",
    "de.saschawillems.vulkanTexturearray",
    "de.saschawillems.vulkanTexturecubemap",
    "de.saschawillems.vulkanTexturemipmapgen",
    "de.saschawillems.vulkanTriangle",
    "de.saschawillems.vulkanVulkanscene"
]

CURR_INDEX = 0

answer = input("Uninstall all vulkan examples from attached device (Y/N)").lower() == 'y'
if answer:
    for app in APP_NAMES:
        print("Uninstalling %s (%d/%d)" % (app, CURR_INDEX, len(APP_NAMES)))
        subprocess.call("adb uninstall %s" % (app))
        CURR_INDEX += 1
    