# Copyright (C) 2016-2024 by Sascha Willems - www.saschawillems.de
# This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)

import argparse
import fileinput
import os
import subprocess
import sys

parser = argparse.ArgumentParser(description='Compile all GLSL shaders')
parser.add_argument('--glslang', type=str, help='path to glslangvalidator executable')
parser.add_argument('--sample', type=str, help='can be used to compile shaders for a single sample only')
parser.add_argument('--g', action='store_true', help='compile with debug symbols')
args = parser.parse_args()

def findGlslang():
    def isExe(path):
        return os.path.isfile(path) and os.access(path, os.X_OK)

    if args.glslang != None and isExe(args.glslang):
        return args.glslang

    exe_name = "glslangValidator"
    if os.name == "nt":
        exe_name += ".exe"

    for exe_dir in os.environ["PATH"].split(os.pathsep):
        full_path = os.path.join(exe_dir, exe_name)
        if isExe(full_path):
            return full_path

    sys.exit("Could not find glslangvalidator executable on PATH, and was not specified with --glslang")

file_extensions = tuple([".vert", ".frag", ".comp", ".geom", ".tesc", ".tese", ".rgen", ".rchit", ".rmiss", ".mesh", ".task"])

compile_single_sample = ""
if args.sample != None:
    compile_single_sample = args.sample
    if (not os.path.isdir(compile_single_sample)):
        print("ERROR: No directory found with name %s" % compile_single_sample)
        exit(-1)

glslang_path = findGlslang()
dir_path = os.path.dirname(os.path.realpath(__file__))
dir_path = dir_path.replace('\\', '/')
for root, dirs, files in os.walk(dir_path):
    folder_name = os.path.basename(root)
    if (compile_single_sample != "" and folder_name != compile_single_sample):
        continue

    for file in files:
        if file.endswith(file_extensions):
            input_file = os.path.join(root, file)
            output_file = input_file + ".spv"

            add_params = ""
            if args.g:
                add_params = "-g"

            # Ray tracing shaders require a different target environment           
            if file.endswith(".rgen") or file.endswith(".rchit") or file.endswith(".rmiss"):
               add_params = add_params + " --target-env vulkan1.2"
            # Same goes for samples that use ray queries
            if root.endswith("rayquery") and file.endswith(".frag"):
                add_params = add_params + " --target-env vulkan1.2"
            # Mesh and task shader also require different settings
            if file.endswith(".mesh") or file.endswith(".task"):
                add_params = add_params + " --target-env spirv1.4"

            res = subprocess.call("%s -V %s -o %s %s" % (glslang_path, input_file, output_file, add_params), shell=True)
            if res != 0:
                sys.exit(res)
