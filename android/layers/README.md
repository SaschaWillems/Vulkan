# Android validation layers

Put the pre-built validation layers into the folder for the architecture you're targeting.

E.g. for armeabi-v7a:

- ./armeabi-v7a/libVkLayer_core_validation.so
- ./armeabi-v7a/libVkLayer_object_tracker.so
- ...

After this, build the example you want validation to be enabled for with the "-validation" flag, e.g.:

```
build pbrtexture -validation
```