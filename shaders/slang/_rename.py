# Copyright (C) 2025 by Sascha Willems - www.saschawillems.de
# This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)

from shutil import move

# To match required file names to fother shading languages that don't support multiple entry points, shader files may need to be renamed for some samples
def checkRenameFiles(samplename):
    mappings = {}
    match samplename:
        case "displacement":
            mappings = {
                "displacement.vert.spv": "base.vert.spv",
                "displacement.frag.spv": "base.frag.spv",
            }        
        case "geometryshader":
            mappings = {
                "normaldebug.vert.spv": "base.vert.spv",
                "normaldebug.frag.spv": "base.frag.spv",
            }
        case "graphicspipelinelibrary":
            mappings = {
                "uber.vert.spv": "shared.vert.spv",
            }             
        case "raytracingbasic":
            mappings = {
                "raytracingbasic.rchit.spv": "closesthit.rchit.spv",
                "raytracingbasic.rmiss.spv": "miss.rmiss.spv",
                "raytracingbasic.rgen.spv": "raygen.rgen.spv",
            }
        case "raytracingcallable":
            mappings = {
                "raytracingcallable.rchit.spv": "closesthit.rchit.spv",
                "raytracingcallable.rmiss.spv": "miss.rmiss.spv",
                "raytracingcallable.rgen.spv": "raygen.rgen.spv",
            }            
        case "raytracinggltf":
            mappings = {
                "raytracinggltf.rchit.spv": "closesthit.rchit.spv",
                "raytracinggltf.rmiss.spv": "miss.rmiss.spv",
                "raytracinggltf.rgen.spv": "raygen.rgen.spv",
                "raytracinggltf.rahit.spv": "anyhit.rahit.spv",
            }
        case "raytracingpositionfetch":
            mappings = {
                "raytracingpositionfetch.rchit.spv": "closesthit.rchit.spv",
                "raytracingpositionfetch.rmiss.spv": "miss.rmiss.spv",
                "raytracingpositionfetch.rgen.spv": "raygen.rgen.spv",
            }                     
        case "raytracingreflections":
            mappings = {
                "raytracingreflections.rchit.spv": "closesthit.rchit.spv",
                "raytracingreflections.rmiss.spv": "miss.rmiss.spv",
                "raytracingreflections.rgen.spv": "raygen.rgen.spv",
            }
        case "raytracingsbtdata":
            mappings = {
                "raytracingsbtdata.rchit.spv": "closesthit.rchit.spv",
                "raytracingsbtdata.rmiss.spv": "miss.rmiss.spv",
                "raytracingsbtdata.rgen.spv": "raygen.rgen.spv",
            }             
        case "raytracingshadows":
            mappings = {
                "raytracingshadows.rchit.spv": "closesthit.rchit.spv",
                "raytracingshadows.rmiss.spv": "miss.rmiss.spv",
                "raytracingshadows.rgen.spv": "raygen.rgen.spv",
            }
        case "raytracingtextures":
            mappings = {
                "raytracingtextures.rchit.spv": "closesthit.rchit.spv",
                "raytracingtextures.rmiss.spv": "miss.rmiss.spv",
                "raytracingtextures.rgen.spv": "raygen.rgen.spv",
                "raytracingtextures.rahit.spv": "anyhit.rahit.spv",
            }            
        case "raytracingintersection":
            mappings = {
                "raytracingintersection.rchit.spv": "closesthit.rchit.spv",
                "raytracingintersection.rmiss.spv": "miss.rmiss.spv",
                "raytracingintersection.rgen.spv": "raygen.rgen.spv",
                "raytracingintersection.rint.spv": "intersection.rint.spv",
            }                   
        case "viewportarray":
            mappings = {
                "scene.geom.spv": "multiview.geom.spv",
            }
    for x, y in mappings.items():
        move(samplename + "\\" + x, samplename + "\\" + y)
