# Copyright (C) 2025 by Sascha Willems - www.saschawillems.de
# This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)

import argparse
import fileinput
import os
import subprocess
import sys

parser = argparse.ArgumentParser(description='Compile all slang shaders')
parser.add_argument('--slangc', type=str, help='path to slangc executable')
args = parser.parse_args()

def findCompiler():
    def isExe(path):
        return os.path.isfile(path) and os.access(path, os.X_OK)

    if args.slangc != None and isExe(args.slangc):
        return args.slangc

    exe_name = "slangc"
    if os.name == "nt":
        exe_name += ".exe"

    for exe_dir in os.environ["PATH"].split(os.pathsep):
        full_path = os.path.join(exe_dir, exe_name)
        if isExe(full_path):
            return full_path

    sys.exit("Could not find slangc executable on PATH, and was not specified with --slangc")

def getShaderStages(filename):
    stages = []
    with open(filename) as f:
        filecontent = f.read()
        if '[shader("vertex")]' in filecontent:
            stages.append("vertex")
        if '[shader("fragment")]' in filecontent:
            stages.append("fragment")
        f.close()
    return stages

compiler_path = findCompiler()

print("Found slang compiler at %s", compiler_path)

dir_path = os.path.dirname(os.path.realpath(__file__))
dir_path = dir_path.replace('\\', '/')
for root, dirs, files in os.walk(dir_path):
    for file in files:
        if file.endswith(".slang"):
            input_file = os.path.join(root, file)
            # Slang can store multiple shader stages in a single file, we need to split into separate SPIR-V files for the sample framework
            stages = getShaderStages(input_file)
            print("Compiling %s" % input_file)
            for stage in stages:
                if (len(stages) > 1):
                    entry_point = stage + "main"
                else:
                    entry_point = "main"
                output_ext = ""
                match stage:
                    case "vertex":
                        output_ext = ".vert"
                    case "fragment":
                        output_ext = ".frag"
                output_file = input_file + output_ext + ".spv"
                output_file = output_file.replace(".slang", "")
                res = subprocess.call("%s %s -profile spirv_1_4 -matrix-layout-column-major -target spirv -o %s -entry %s -stage %s" % (compiler_path, input_file, output_file, entry_point, stage), shell=True)
                print(output_file)
                
            # res = subprocess.call("%s %s -profile spirv_1_4 -matrix-layout-column-major -target spirv -o %s -entry %s" % (compiler_path, input_file, output_file, "main"), shell=True)
            # if res != 0:
                # sys.exit(res)