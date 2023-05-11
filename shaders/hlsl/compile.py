# Copyright 2020 Google LLC

import argparse
import fileinput
import os
import subprocess
import sys

parser = argparse.ArgumentParser(description='Compile all .hlsl shaders')
parser.add_argument('--dxc', type=str, help='path to DXC executable')
args = parser.parse_args()

def findDXC():
    def isExe(path):
        return os.path.isfile(path) and os.access(path, os.X_OK)

    if args.dxc != None and isExe(args.dxc):
        return args.dxc

    exe_name = "dxc"
    if os.name == "nt":
        exe_name += ".exe"

    for exe_dir in os.environ["PATH"].split(os.pathsep):
        full_path = os.path.join(exe_dir, exe_name)
        if isExe(full_path):
            return full_path

    sys.exit("Could not find DXC executable on PATH, and was not specified with --dxc")

dxc_path = findDXC()
dir_path = os.path.dirname(os.path.realpath(__file__))
dir_path = dir_path.replace('\\', '/')
for root, dirs, files in os.walk(dir_path):
    for file in files:
        if file.endswith(".vert") or file.endswith(".frag") or file.endswith(".comp") or file.endswith(".geom") or file.endswith(".tesc") or file.endswith(".tese") or file.endswith(".rgen") or file.endswith(".rchit") or file.endswith(".rmiss"):
            hlsl_file = os.path.join(root, file)
            spv_out = hlsl_file + ".spv"

            target = ''
            profile = ''
            if(hlsl_file.find('.vert') != -1):
                profile = 'vs_6_1'
            elif(hlsl_file.find('.frag') != -1):
                profile = 'ps_6_1'
            elif(hlsl_file.find('.comp') != -1):
                profile = 'cs_6_1'
            elif(hlsl_file.find('.geom') != -1):
                profile = 'gs_6_1'
            elif(hlsl_file.find('.tesc') != -1):
                profile = 'hs_6_1'
            elif(hlsl_file.find('.tese') != -1):
                profile = 'ds_6_1'
            elif(hlsl_file.find('.rgen') != -1 or
				hlsl_file.find('.rchit') != -1 or
				hlsl_file.find('.rmiss') != -1):
                target='-fspv-target-env=vulkan1.2'
                profile = 'lib_6_3'

            print('Compiling %s' % (hlsl_file))
            subprocess.check_output([
                dxc_path,
                '-spirv',
                '-T', profile,
                '-E', 'main',
                '-fspv-extension=SPV_KHR_ray_tracing',
                '-fspv-extension=SPV_KHR_multiview',
                '-fspv-extension=SPV_KHR_shader_draw_parameters',
                '-fspv-extension=SPV_EXT_descriptor_indexing',
                target,
                hlsl_file,
                '-Fo', spv_out])
