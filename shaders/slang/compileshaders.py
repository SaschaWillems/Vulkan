# Copyright (C) 2025 by Sascha Willems - www.saschawillems.de
# This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)

import argparse
import fileinput
import os
import subprocess
import sys
from _rename import *

parser = argparse.ArgumentParser(description='Compile all slang shaders')
parser.add_argument('--slangc', type=str, help='path to slangc executable')
parser.add_argument('--sample', type=str, help='can be used to compile shaders for a single sample only')
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
        if '[shader("raygeneration")]' in filecontent:
            stages.append("raygeneration")
        if '[shader("miss")]' in filecontent:
            stages.append("miss")
        if '[shader("closesthit")]' in filecontent:
            stages.append("closesthit")
        if '[shader("callable")]' in filecontent:
            stages.append("callable")
        if '[shader("intersection")]' in filecontent:
            stages.append("intersection")
        if '[shader("anyhit")]' in filecontent:
            stages.append("anyhit")
        if '[shader("compute")]' in filecontent:
            stages.append("compute")
        if '[shader("amplification")]' in filecontent:
            stages.append("amplification")
        if '[shader("mesh")]' in filecontent:
            stages.append("mesh")
        if '[shader("geometry")]' in filecontent:
            stages.append("geometry")
        if '[shader("hull")]' in filecontent:
            stages.append("hull")            
        if '[shader("domain")]' in filecontent:
            stages.append("domain")            
        f.close()
    return stages

compiler_path = findCompiler()

print("Found slang compiler at %s", compiler_path)

compile_single_sample = ""
if args.sample != None:
    compile_single_sample = args.sample
    if (not os.path.isdir(compile_single_sample)):
        print("ERROR: No directory found with name %s" % compile_single_sample)
        exit(-1)

dir_path = os.path.dirname(os.path.realpath(__file__))
dir_path = dir_path.replace('\\', '/')
for root, dirs, files in os.walk(dir_path):
    folder_name = os.path.basename(root)
    if (compile_single_sample != "" and folder_name != compile_single_sample):
        continue
    for file in files:
        if file.endswith(".slang"):
            input_file = os.path.join(root, file)
            # Slang can store multiple shader stages in a single file, we need to split into separate SPIR-V files for the sample framework
            stages = getShaderStages(input_file)
            print("Compiling %s" % input_file)
            output_base_file_name = input_file
            for stage in stages:
                entry_point = stage + "Main"
                output_ext = ""
                match stage:
                    case "vertex":
                        output_ext = ".vert"
                    case "fragment":
                        output_ext = ".frag"
                    case "raygeneration":
                        output_ext = ".rgen"
                    case "miss":
                        output_ext = ".rmiss"
                    case "closesthit":
                        output_ext = ".rchit"
                    case "callable":
                        output_ext = ".rcall"
                    case "intersection":
                        output_ext = ".rint"
                    case "anyhit":
                        output_ext = ".rahit"
                    case "compute":
                        output_ext = ".comp"
                    case "mesh":
                        output_ext = ".mesh"
                    case "amplification":
                        output_ext = ".task"
                    case "geometry":
                        output_ext = ".geom"
                    case "hull":
                        output_ext = ".tesc"
                    case "domain":
                        output_ext = ".tese"
                output_file = output_base_file_name + output_ext + ".spv"
                output_file = output_file.replace(".slang", "")
                print(output_file)
                res = subprocess.call("%s %s -profile spirv_1_4 -matrix-layout-column-major -target spirv -o %s -entry %s -stage %s -warnings-disable 39001" % (compiler_path, input_file, output_file, entry_point, stage), shell=True)
                if res != 0:
                    print("Error %s", res)
                    sys.exit(res)
    checkRenameFiles(folder_name)