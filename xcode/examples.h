/*
 *  examples.h
 *
 *  Copyright (c) 2016-2017 The Brenwill Workshop Ltd.
 *  This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 *
 *
 * Loads the appropriate example code, as indicated by the appropriate compiler build setting below.
 *
 * To select an example to run, define one (and only one) of the macros below, either by
 * adding a #define XXX statement at the top of this file, or more flexibily, by adding the
 * macro value to the Preprocessor Macros (aka GCC_PREPROCESSOR_DEFINITIONS) compiler setting.
 *
 * To add a compiler setting, select the project in the Xcode Project Navigator panel,
 * select the Build Settings panel, and add the value to the Preprocessor Macros
 * (aka GCC_PREPROCESSOR_DEFINITIONS) entry.
 *
 * For example, to run the pipelines example, you would add the MVK_pipelines define macro
 * to the Preprocessor Macros (aka GCC_PREPROCESSOR_DEFINITIONS) entry of the Xcode project,
 * overwriting any other value there.
 *
 * If you choose to add a #define statement to this file, be sure to clear the existing macro
 * from the Preprocessor Macros (aka GCC_PREPROCESSOR_DEFINITIONS) compiler setting in Xcode.
 */


// In the list below, the comments indicate entries that,
// under certain conditions, that may not run as expected.

// Uncomment the next line and select example here if not using a Preprocessor Macro to define example
//#define MVK_vulkanscene

// COMMON  - Include VulkanglTFModel.cpp in all examples other than ones that already include/customize tiny_gltf.h directly
#if !defined(MVK_gltfloading) && !defined(MVK_gltfskinning) && !defined(MVK_gltfscenerendering) && !defined(MVK_vertexattributes)
#	include "../base/VulkanglTFModel.cpp"
#endif


// BASICS

#ifdef MVK_triangle
#   include "../examples/triangle/triangle.cpp"
#endif

#ifdef MVK_pipelines
#   include "../examples/pipelines/pipelines.cpp"
#endif

#ifdef MVK_texture
#   include "../examples/texture/texture.cpp"
#endif

#ifdef MVK_texture3d
#   include "../examples/texture3d/texture3d.cpp"
#endif

#ifdef MVK_texturecubemap
#   include "../examples/texturecubemap/texturecubemap.cpp"
#endif

#ifdef MVK_texturecubemaparray
#   include "../examples/texturecubemaparray/texturecubemaparray.cpp"
#endif

#ifdef MVK_texturearray
#   include "../examples/texturearray/texturearray.cpp"
#endif

#ifdef MVK_dynamicuniformbuffer
#   include "../examples/dynamicuniformbuffer/dynamicuniformbuffer.cpp"
#endif

#ifdef MVK_inlineuniformblocks
#   include "../examples/inlineuniformblocks/inlineuniformblocks.cpp"
#endif

#ifdef MVK_inputattachments
#   include "../examples/inputattachments/inputattachments.cpp"
#endif

#ifdef MVK_negativeviewportheight
#   include "../examples/negativeviewportheight/negativeviewportheight.cpp"
#endif

#ifdef MVK_pushconstants
#   include "../examples/pushconstants/pushconstants.cpp"
#endif

#ifdef MVK_specializationconstants
#   include "../examples/specializationconstants/specializationconstants.cpp"
#endif

#ifdef MVK_offscreen
#   include "../examples/offscreen/offscreen.cpp"
#endif

#ifdef MVK_stencilbuffer
#   include "../examples/stencilbuffer/stencilbuffer.cpp"
#endif

#ifdef MVK_subpasses
#   include "../examples/subpasses/subpasses.cpp"
#endif

#ifdef MVK_radialblur
#   include "../examples/radialblur/radialblur.cpp"
#endif

#ifdef MVK_textoverlay
#   include "../examples/textoverlay/textoverlay.cpp"
#endif

#ifdef MVK_particlefire
#   include "../examples/particlefire/particlefire.cpp"
#endif

// No headless target when using xcode examples project, builds/runs fine using vulkanExamples project.
#ifdef MVK_renderheadless
#   include "../examples/renderheadless/renderheadless.cpp"
#endif


// ADVANCED

#ifdef MVK_multithreading
#   include "../examples/multithreading/multithreading.cpp"
#endif

#ifdef MVK_gltfscenerendering
#   include "../examples/gltfscenerendering/gltfscenerendering.cpp"
#endif

#ifdef MVK_multiview
#   include "../examples/multiview/multiview.cpp"
#endif

#ifdef MVK_instancing
#   include "../examples/instancing/instancing.cpp"
#endif

#ifdef MVK_indirectdraw
#   include "../examples/indirectdraw/indirectdraw.cpp"
#endif

#ifdef MVK_hdr
#   include "../examples/hdr/hdr.cpp"
#endif

#ifdef MVK_occlusionquery
#   include "../examples/occlusionquery/occlusionquery.cpp"
#endif

#ifdef MVK_texturemipmapgen
#   include "../examples/texturemipmapgen/texturemipmapgen.cpp"
#endif

// Does not run.  Sparse binding not supported by MoltenVK/Metal.
#ifdef MVK_texturesparseresidency
#   include "../examples/texturesparseresidency/texturesparseresidency.cpp"
#endif

#ifdef MVK_multisampling
#   include "../examples/multisampling/multisampling.cpp"
#endif

#ifdef MVK_shadowmapping
#   include "../examples/shadowmapping/shadowmapping.cpp"
#endif

#ifdef MVK_shadowmappingcascade
#   include "../examples/shadowmappingcascade/shadowmappingcascade.cpp"
#endif

#ifdef MVK_shadowmappingomni
#   include "../examples/shadowmappingomni/shadowmappingomni.cpp"
#endif

#ifdef MVK_gltfloading
#   include "../examples/gltfloading/gltfloading.cpp"
#endif

#ifdef MVK_gltfskinning
#   include "../examples/gltfskinning/gltfskinning.cpp"
#endif

#ifdef MVK_bloom
#   include "../examples/bloom/bloom.cpp"
#endif

#ifdef MVK_vertexattributes
#   include "../examples/vertexattributes/vertexattributes.cpp"
#endif

// Runs, but some Apple GPUs may not support stores and atomic operations in the fragment stage.
#ifdef MVK_oit
#   include "../examples/oit/oit.cpp"
#endif


// DEFERRED

#ifdef MVK_deferred
#   include "../examples/deferred/deferred.cpp"
#endif

#ifdef MVK_deferredmultisampling
#   include "../examples/deferredmultisampling/deferredmultisampling.cpp"
#endif

#ifdef MVK_ssao
#   include "../examples/ssao/ssao.cpp"
#endif


// DESCRIPTORS

#ifdef MVK_descriptorsets
#   include "../examples/descriptorsets/descriptorsets.cpp"
#endif

#ifdef MVK_pushdescriptors
#   include "../examples/pushdescriptors/pushdescriptors.cpp"
#endif

// Runs on macOS 11.0 or later with Metal argument buffers enabled.  Not yet supported on iOS.
#ifdef MVK_descriptorindexing
#   include "../examples/descriptorindexing/descriptorindexing.cpp"
#endif


// PHYSICALLY BASED RENDERING

#ifdef MVK_pbrbasic
#   include "../examples/pbrbasic/pbrbasic.cpp"
#endif

#ifdef MVK_pbribl
#   include "../examples/pbribl/pbribl.cpp"
#endif

#ifdef MVK_pbrtexture
#   include "../examples/pbrtexture/pbrtexture.cpp"
#endif


// RAY TRACING - Currently unsupported by MoltenVK/Metal

// Does not run.  Missing Vulkan extensions for ray tracing
#ifdef MVK_rayquery
#   include "../examples/rayquery/rayquery.cpp"
#endif

// Does not run.  Missing Vulkan extensions for ray tracing
#ifdef MVK_raytracingbasic
#   include "../examples/raytracingbasic/raytracingbasic.cpp"
#endif

// Does not run.  Missing Vulkan extensions for ray tracing
#ifdef MVK_raytracingcallable
#   include "../examples/raytracingcallable/raytracingcallable.cpp"
#endif

// Does not run.  Missing Vulkan extensions for ray tracing
#ifdef MVK_raytracingreflections
#   include "../examples/raytracingreflections/raytracingreflections.cpp"
#endif

// Does not run.  Missing Vulkan extensions for ray tracing
#ifdef MVK_raytracingshadows
#   include "../examples/raytracingshadows/raytracingshadows.cpp"
#endif


// COMPUTE

#ifdef MVK_computeparticles
#   include "../examples/computeparticles/computeparticles.cpp"
#endif

#ifdef MVK_computenbody
#   include "../examples/computenbody/computenbody.cpp"
#endif

#ifdef MVK_computeraytracing
#   include "../examples/computeraytracing/computeraytracing.cpp"
#endif

#ifdef MVK_computecullandlod
#   include "../examples/computecullandlod/computecullandlod.cpp"
#endif

#ifdef MVK_computeshader
#   include "../examples/computeshader/computeshader.cpp"
#endif

#ifdef MVK_computecloth
#   include "../examples/computecloth/computecloth.cpp"
#endif

// No headless target when using xcode examples project, builds/runs fine using vulkanExamples project.
#ifdef MVK_computeheadless
#   include "../examples/computeheadless/computeheadless.cpp"
#endif


// TESSELLATION

#ifdef MVK_terraintessellation
#   include "../examples/terraintessellation/terraintessellation.cpp"
#endif

#ifdef MVK_tessellation
#   include "../examples/tessellation/tessellation.cpp"
#endif

#ifdef MVK_displacement
#   include "../examples/displacement/displacement.cpp"
#endif


// GEOMETRY SHADER - Unsupported by Metal

// Does not run. Metal does not support geometry shaders.
#ifdef MVK_geometryshader
#   include "../examples/geometryshader/geometryshader.cpp"
#endif

// Does not run. Metal does not support geometry shaders.
#ifdef MVK_deferredshadows
#   include "../examples/deferredshadows/deferredshadows.cpp"
#endif

// Does not run. Metal does not support geometry shaders.
#ifdef MVK_viewportarray
#   include "../examples/viewportarray/viewportarray.cpp"
#endif


// EXTENSIONS

// Does not run. Requires VK_EXT_conditional_rendering.
#ifdef MVK_conditionalrender
#   include "../examples/conditionalrender/conditionalrender.cpp"
#endif

// Does not run. Requires VK_EXT_conservative_rasterization.
#ifdef MVK_conservativeraster
#   include "../examples/conservativeraster/conservativeraster.cpp"
#endif

// Does not run. Requires VK_NV_shading_rate_image.
#ifdef MVK_variablerateshading
#   include "../examples/variablerateshading/variablerateshading.cpp"
#endif

// Does not run. Requires VK_KHR_pipeline_library and VK_EXT_graphics_pipeline_library.
#ifdef MVK_graphicspipelinelibrary
#   include "../examples/graphicspipelinelibrary/graphicspipelinelibrary.cpp"
#endif

// Runs on MoltenVK 1.1.10 (SDK 1.3.216) or later. Requires VK_KHR_dynamic_rendering and dependencies.
#ifdef MVK_dynamicrendering
#   include "../examples/dynamicrendering/dynamicrendering.cpp"
#endif

// Runs. Requires VK_EXT_debug_marker.
#ifdef MVK_debugmarker
#   include "../examples/debugmarker/debugmarker.cpp"
#endif


// MISC

#ifdef MVK_screenshot
#   include "../examples/screenshot/screenshot.cpp"
#endif

#ifdef MVK_parallaxmapping
#   include "../examples/parallaxmapping/parallaxmapping.cpp"
#endif

#ifdef MVK_sphericalenvmapping
#   include "../examples/sphericalenvmapping/sphericalenvmapping.cpp"
#endif

#ifdef MVK_gears
#   include "../examples/gears/gears.cpp"
#   include "../examples/gears/vulkangear.cpp"
#endif

#ifdef MVK_distancefieldfonts
#   include "../examples/distancefieldfonts/distancefieldfonts.cpp"
#endif

// Does not run.  MoltenVK/Metal does not support pipeline statistics.
#ifdef MVK_pipelinestatistics
#   include "../examples/pipelinestatistics/pipelinestatistics.cpp"
#endif

#ifdef MVK_imgui
#   include "../examples/imgui/main.cpp"
#endif

#ifdef MVK_vulkanscene
#   include "../examples/vulkanscene/vulkanscene.cpp"
#endif
