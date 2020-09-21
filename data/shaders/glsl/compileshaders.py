import argparse
import fileinput
import os
import subprocess
import sys

parser = argparse.ArgumentParser(description='Compile all GLSL shaders')
parser.add_argument('--glslang', type=str, help='path to glslangvalidator executable')
parser.add_argument('--g', action='store_true', help='compile with debug symbols')
args = parser.parse_args()

def findGlslang():
    def isExe(path):
        return os.path.isfile(path) and os.access(path, os.X_OK)

    if args.glslang != None and isExe(args.glslang):
        return args.glslang

    exe_name = "glslangvalidator"
    if os.name == "nt":
        exe_name += ".exe"

    for exe_dir in os.environ["PATH"].split(os.pathsep):
        full_path = os.path.join(exe_dir, exe_name)
        if isExe(full_path):
            return full_path

    sys.exit("Could not find DXC executable on PATH, and was not specified with --dxc")

glslang_path = findGlslang()
dir_path = os.path.dirname(os.path.realpath(__file__))
dir_path = dir_path.replace('\\', '/')
for root, dirs, files in os.walk(dir_path):
    for file in files:
        if file.endswith(".vert") or file.endswith(".frag") or file.endswith(".comp") or file.endswith(".geom") or file.endswith(".tesc") or file.endswith(".tese") or file.endswith(".rgen") or file.endswith(".rchit") or file.endswith(".rmiss"):
            input_file = os.path.join(root, file)
            output_file = input_file + ".spv"

            add_params = ""
            if args.g:
                add_params = "-g"

            if file.endswith(".rgen") or file.endswith(".rchit") or file.endswith(".rmiss"):
               add_params = add_params + " --target-env vulkan1.2"

            res = subprocess.call("%s -V %s -o %s %s" % (glslang_path, input_file, output_file, add_params), shell=True)
            # res = subprocess.call([glslang_path, '-V', input_file, '-o', output_file, add_params], shell=True)
            if res != 0:
                sys.exit()