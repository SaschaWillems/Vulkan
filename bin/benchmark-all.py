# Benchmark all examples
import subprocess
import sys
import os

EXAMPLES = [
	"bloom",
	"computecloth",
	"computecullandlod",
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
	"pbrtexture",
	"pipelines",
	"pushconstants",
	"radialblur",
	"raytracing",
	"scenerendering",
	"shadowmapping",
	"shadowmappingomni",
	"skeletalanimation",
	"specializationconstants",
	"sphericalenvmapping",
	"ssao",
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
	"texturesparseresidency",
	"triangle",
	"viewportarray",
	"vulkanscene"
]

COLOR_GREEN = '\033[92m'
COLOR_END = '\033[0m'

CURR_INDEX = 0

ARGS = "-fullscreen -b"

print("Benchmarking all examples...")

os.makedirs("./benchmark", exist_ok=True)

for example in EXAMPLES:
	print("Running %s (%d/%d) in benchmark mode" % (example, CURR_INDEX+1, len(EXAMPLES)))
	RESULT_CODE = subprocess.call("%s -fullscreen -b ./benchmark/%s" % (example, example)) 
	if RESULT_CODE == 0:
		print("\tResults written to ./benchmark/%s.csv" % example)
	else:
		print("\tError, result code = %d" % RESULT_CODE)
	CURR_INDEX += 1

print("Benchmark run finished")
