/*
 * examples.h
 *
 * Copyright (c) 2014-2017 The Brenwill Workshop Ltd. All rights reserved.
 * http://www.brenwill.com
 */


/** 
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
 * overwriting any otheor value there.
 *
 * If you choose to add a #define statement to this file, be sure to clear the existing macro
 * from the Preprocessor Macros (aka GCC_PREPROCESSOR_DEFINITIONS) compiler setting in Xcode.
 */


// In the list below, the comments indicate entries that do not currently run correctly on
// one or both of iOS and macOS, and the comment indicates the problem that is encountered.
// Fixes are on the way.


// BASICS

#ifdef MVK_pipelines
#	include "../pipelines/pipelines.cpp"
#endif

#ifdef MVK_texture                          // Bad access
#	include "../texture/texture.cpp"
#endif

#ifdef MVK_texturecubemap                   // mat4 passed as input
#	include "../texturecubemap/texturecubemap.cpp"
#endif

#ifdef MVK_texturearray                     // Buffer binding error
#	include "../texturearray/texturearray.cpp"
#endif

#ifdef MVK_mesh
#	include "../mesh/mesh.cpp"
#endif

#ifdef MVK_dynamicuniformbuffer             // Bad access
#	include "../dynamicuniformbuffer/dynamicuniformbuffer.cpp"
#endif

#ifdef MVK_pushconstants                    // Array in shader stage_in breaks shader
#	include "../pushconstants/pushconstants.cpp"
#endif

#ifdef MVK_specializationconstants          // Specialization constants not recognized in shader
#	include "../specializationconstants/specializationconstants.cpp"
#endif

#ifdef MVK_offscreen                        // Bad access on iOS after 'd' key is pressed
#	include "../offscreen/offscreen.cpp"
#endif

#ifdef MVK_radialblur
#	include "../radialblur/radialblur.cpp"
#endif

#ifdef MVK_textoverlay
#	include "../textoverlay/textoverlay.cpp"
#endif

#ifdef MVK_particlefire
#	include "../particlefire/particlefire.cpp"
#endif


// ADVANCED

#ifdef MVK_multithreading
#	include "../multithreading/multithreading.cpp"
#endif

#ifdef MVK_scenerendering                   // Bad access on macOS
#	include "../scenerendering/scenerendering.cpp"
#endif

#ifdef MVK_instancing
#	include "../instancing/instancing.cpp"
#endif

#ifdef MVK_indirectdraw
#	include "../indirectdraw/indirectdraw.cpp"
#endif

#ifdef MVK_hdr                              // mat4 passed as input
#	include "../hdr/hdr.cpp"
#endif

#ifdef MVK_occlusionquery                   // Runs but exhausts 4096 capacity dynamic buffer
#	include "../occlusionquery/occlusionquery.cpp"
#endif

#ifdef MVK_texturemipmapgen                 // SPIRV->GLSL conversion error
#	include "../texturemipmapgen/texturemipmapgen.cpp"
#endif

#ifdef MVK_multisampling                    // Multisampling too low on iOS
#	include "../multisampling/multisampling.cpp"
#endif

#ifdef MVK_shadowmapping                    // Bad access on iOS
#	include "../shadowmapping/shadowmapping.cpp"
#endif

#ifdef MVK_shadowmappingomni                // Bad access on iOS
#	include "../shadowmappingomni/shadowmappingomni.cpp"
#endif

#ifdef MVK_skeletalanimation                // Bad access on macOS
#	include "../skeletalanimation/skeletalanimation.cpp"
#endif

#ifdef MVK_bloom                            // Bad access on iOS
#	include "../bloom/bloom.cpp"
#endif

#ifdef MVK_deferred                         // buffer overload
#	include "../deferred/deferred.cpp"
#endif

#ifdef MVK_deferredshadows                  // Geometry shaders not available in Metal
#	include "../deferredshadows/deferredshadows.cpp"
#endif

#ifdef MVK_ssao                             // SPIRV->MSL conversion error
#	include "../ssao/ssao.cpp"
#endif


// COMPUTE - Currently unsupported by MoltenVK


// TESSELLATION - Currently unsupported by MoltenVK


// GEOMETRY SHADER - Unsupported by Metal


// EXTENSIONS - Currently unsupported by MoltenVK


// MISC

#ifdef MVK_parallaxmapping
#	include "../parallaxmapping/parallaxmapping.cpp"
#endif

#ifdef MVK_sphericalenvmapping
#	include "../sphericalenvmapping/sphericalenvmapping.cpp"
#endif

#ifdef MVK_gears
#	include "../gears/gears.cpp"
#endif

#ifdef MVK_distancefieldfonts           // Endless loop during loading on macOS
#	include "../distancefieldfonts/distancefieldfonts.cpp"
#endif

#ifdef MVK_vulkanscene
#	include "../vulkanscene/vulkanscene.cpp"
#endif

