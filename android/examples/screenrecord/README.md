This example demonstrates video screen recording on Android by using AMediaCodec API

0 Create codec surface AMediaCodec_createInputSurface
1 Create Vulkan surface from codec surface  
2 Blit images from display surface to codec surface


# How to retrieve and play the encoded file?

```shell
adb shell run-as de.saschawillems.vulkanScreenRecord cat video.h264 >video.h264
ffplay video.h264
```