# Copyright (C) 2025 by Sascha Willems - www.saschawillems.de
# This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)

from shutil import move

# To match required file names to fother shading languages that don't support multiple entry points, shader files may need to be renamed for some samples
def checkRenameFiles(samplename):
    mappings = {}
    match samplename:
        case "raytracingbasic":
            mappings = {
                "raytracingbasic.rchit.spv": "closesthit.rchit.spv",
                "raytracingbasic.rmiss.spv": "miss.rmiss.spv",
                "raytracingbasic.rgen.spv": "raygen.rgen.spv",
            }
        case "raytracingreflections":
            mappings = {
                "raytracingreflections.rchit.spv": "closesthit.rchit.spv",
                "raytracingreflections.rmiss.spv": "miss.rmiss.spv",
                "raytracingreflections.rgen.spv": "raygen.rgen.spv",
            }  
    for x, y in mappings.items():
        move(samplename + "\\" + x, samplename + "\\" + y)
