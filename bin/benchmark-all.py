# Benchmark all examples
import subprocess
import sys
import os
import platform

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

CURR_INDEX = 0

ARGS = "-fullscreen -b"

print("Benchmarking all examples...")

os.makedirs("./benchmark", exist_ok=True)

for example in EXAMPLES:
	print("---- (%d/%d) Running %s in benchmark mode ----" % (CURR_INDEX+1, len(EXAMPLES), example))
	if platform.system() == 'Linux':
		RESULT_CODE = subprocess.call("./%s %s -bf ./benchmark/%s.csv 5" % (example, ARGS, example), shell=True)
	else:
		RESULT_CODE = subprocess.call("%s %s -bf ./benchmark/%s.csv 5" % (example, ARGS, example))
	if RESULT_CODE == 0:
		print("Results written to ./benchmark/%s.csv" % example)
	else:
		print("Error, result code = %d" % RESULT_CODE)
	CURR_INDEX += 1

print("Benchmark run finished")
