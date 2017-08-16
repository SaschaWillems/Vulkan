# Build all examples
# Pass -deploy to also install on connected device
import subprocess
import sys

EXAMPLES = [
    "bloom",
    "computecullandlod",
    "computecloth",
    "computenbody",
    "computeparticles",
    "computeshader",
    "debugmarker",
    "deferred",
    "deferredmultisampling",
    "deferredshadows",
    "displacement",
    "distancefieldfonts",
    "dynamicuniformbuffer",
    "gears",
    "geometryshader",
    "hdr",
    "imgui",
    "indirectdraw",
    "instancing",
    "mesh",
    "multisampling",
    "multithreading",
    "occlusionquery",
    "offscreen",
    "parallaxmapping",
    "particlefire",
    "pbrbasic",
    "pbribl",
    "pipelines",
    "pushconstants",
    "radialblur",
    "raytracing",
    "ssao",
    "scenerendering",
    "shadowmapping",
    "shadowmappingomni",
    "skeletalanimation",
    "specializationconstants",
    "sphericalenvmapping",
    "stencilbuffer",
    "subpasses",
    "terraintessellation",
    "tessellation",
    "textoverlay",
    "texture",
    "texture3d",
    "texturearray",
    "texturecubemap",
    "texturemipmapgen",
    "triangle",
    "vulkanscene"
]

COLOR_GREEN = '\033[92m'
COLOR_END = '\033[0m'

CURR_INDEX = 0

BUILD_ARGUMENTS = ""
for arg in sys.argv[1:]:
    if arg == "-deploy":
        BUILD_ARGUMENTS += "-deploy"
    if arg == "-validation":
        BUILD_ARGUMENTS += "-validation"

print("Building all examples...")

for example in EXAMPLES:
    print(COLOR_GREEN + "Building %s (%d/%d)" % (example, CURR_INDEX, len(EXAMPLES)) + COLOR_END)
    if subprocess.call(("python build.py %s %s" % (example, BUILD_ARGUMENTS)).split(' ')) != 0:
        print("Error during build process for %s" % example)
        sys.exit(-1)
    CURR_INDEX += 1

print("Successfully build %d examples" % CURR_INDEX)
