# Demo meshes

This directory contains the models used in the demos and examples.

Some of them are done by myself from scratch, others are based on existing models.

## License

Unless noted otherwise, the models are free to use under the terms of the [Creative Commons Attribution 3.0 license](http://creativecommons.org/licenses/by/3.0/).

If you use them in your (public) work, please drop me a line so I can check it out ;)

## Formats

Models are always provided as .X (DirectX file format, binary version), as this format supports everything necessary.

If possible (max. 64k triangles) a .3DS file is also provided, if the model has more than 64k triangles a .OBJ (Wavefront) version including the material libary (.mtl) is also provided.

If you need the models in a different format (that's common) please drop me a line.

## Rendering

The demos and examples use [ASSIMP](http://assimp.sourceforge.net/) for loading the models, all models were tested for correct display using a sample C++ model renderer.

Note that the models are usually made up of several meshes due to different materials.

Also note that the scale of the models is not unified, so the general scale may differ. I usually do a normalization of model scaling upon loading models into my examples.

## Vulkan Logo
<img src="./images/vulkanlogo.png" alt="Chinese dragon" width="192px">

A 3D rendition of the official Vulkan(tm) Logo. I made this based on the 2D logo, please note that it's not 100% perfect, e.g. the arc is slightly different.

**Important** : Please regard Khronos' [API logo usage and word mark guidelines](https://www.khronos.org/legal/trademarks/) when using this!

Triangle count : 4,348

## Angry teapot
<img src="./images/angryteapot.png" alt="Angry teapot" width="192px">

Pretty much what it's name suggests. It's an angry teapot, based on the one from the (first) Vulkan T-Shirt Logo.

Made by me from scratch. Just some teapot with angry looking eyes.

Triangle count : 5,564

## Chinese dragon
<img src="./images/chinesedragon.png" alt="Chinese dragon" width="192px">

Based on the Stanford Dragon : http://graphics.stanford.edu/data/3Dscanrep/

(Please read their note on [inappropriate uses for this model](http://graphics.stanford.edu/data/3Dscanrep/#uses))

I did some optimizations on the model to lower the triangle count without loosing too mich detail and put some spheres in for the eyes and under the claw to make them pop out a bit more.

Triangle count : 102,880

## Tactical bunny
<img src="./images/tacticalbunny.png" alt="Chinese dragon" width="192px">

Based on the Stanford Bunny : http://graphics.stanford.edu/data/3Dscanrep/

Reduces vertex count, added some angry eyes (the one's from the angry teapot) and added a tactical helmet, so you get a bunny ready to take down some terrorists here.

The helmet is a slightly modified version of [this model](http://opengameart.org/content/helmet) by Flatlander, licensed under the [CC-BY 3.0](http://creativecommons.org/licenses/by/3.0/).

Triangle count : 42,210
