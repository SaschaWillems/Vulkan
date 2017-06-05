# Additional asset pack

Newer assets (textures and models) will no longer be added to the repository in order to keep it's size down. Especially HDR assets tend to be much larger than most of the ldr textures and compressing them is problematic due to the multi-platform target of the examples (Not all platforms support compressed HDR texture formats).

So these are provided as a separate download required to run some of the newer examples.

Examples that require assets from this pack will have a note in the header:
```cpp
/*
* Vulkan Example
*
* Note: Requires the separate asset pack (see data/README.md)
*
*/
```

## Getting the asset pack

### Option 1: Run the python script

Run the [download_assets.py](../download_assets.py) python script which will download the asset pack and unpacks it into the appropriate folder.

### Option 2: Manual download

Download the asset pack from [http://vulkan.gpuinfo.org/downloads/vulkan_asset_pack.zip](http://vulkan.gpuinfo.org/downloads/vulkan_asset_pack.zip) and extract it in the ```data``` directory.