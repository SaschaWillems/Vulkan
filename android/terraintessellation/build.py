import os
import shutil
import subprocess
import sys
import glob

APK_NAME = "vulkanTerraintessellation"
SHADER_DIR = "terraintessellation"
ASSETS_MODELS = ["geosphere.obj"]
ASSETS_TEXTURES = ["skysphere_bc3_unorm.ktx", "terrain_texturearray_bc3_unorm.ktx", "skysphere_astc_8x8_unorm.ktx", "terrain_texturearray_astc_8x8_unorm.ktx", "skysphere_etc2_unorm.ktx", "terrain_texturearray_etc2_unorm.ktx"]

if subprocess.call("ndk-build", shell=True) == 0:   
    print("Build successful")

    # Assets
    os.makedirs("./assets/shaders/base", exist_ok=True)
    os.makedirs("./assets/shaders/%s" % SHADER_DIR, exist_ok=True)
    os.makedirs("./assets/textures", exist_ok=True)           
    os.makedirs("./assets/models", exist_ok=True)           
    os.makedirs("./res/drawable", exist_ok=True)

    # Shaders    
    # Base
    for file in glob.glob("../../data/shaders/base/*.spv"):
        shutil.copy(file, "./assets/shaders/base")    
    # Sample
    for file in glob.glob("../../data/shaders/%s/*.spv" %SHADER_DIR):
        shutil.copy(file, "./assets/shaders/%s" % SHADER_DIR)    
    # Textures
    for file in ASSETS_TEXTURES:
        shutil.copy("../../data/textures/%s" % file, "./assets/textures")        
    # Models
    for file in ASSETS_MODELS:
        shutil.copy("../../data/models/%s" % file, "./assets/models")

    # Icon
    shutil.copy("../../android/images/icon.png", "./res/drawable")      

    if subprocess.call("ant debug -Dout.final.file=%s.apk" % APK_NAME, shell=True) == 0:
        if len(sys.argv) > 1:
            if sys.argv[1] == "-deploy":
                if subprocess.call("adb install -r %s.apk" % APK_NAME, shell=True) != 0:
                    print("Could not deploy to device!")
    else:
        print("Error during build process!")
else:
    print("Error building project!")
