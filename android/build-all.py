#!/usr/bin/env python3

import argparse
import subprocess
import sys

import build

EXAMPLES = [
    "bloom",
    "computecullandlod",
    "computecloth",
    "computeheadless",
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
    "renderheadless",
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
    "viewportarray",
    "vulkanscene"
]

COLOR_GREEN = '\033[92m'
COLOR_END = '\033[0m'

CURR_INDEX = 0

parser = argparse.ArgumentParser()
parser.add_argument('-deploy', default=False, action='store_true', help="install examples on device")
parser.add_argument('-validation', default=False, action='store_true')
args = parser.parse_args()

print("Building all examples...")

for example in EXAMPLES:
    print(COLOR_GREEN + "Building %s (%d/%d)" % (example, CURR_INDEX, len(EXAMPLES)) + COLOR_END)
    if not build.main(example, args.deploy, args.validation):
        print("Error during build process for %s" % example)
        sys.exit(-1)
    CURR_INDEX += 1

print("Successfully build %d examples" % CURR_INDEX)
