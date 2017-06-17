# Single example build and deploy script
import os
import subprocess
import sys
import shutil
import glob
import json

# Android SDK version used
SDK_VERSION = "android-23"

PROJECT_FOLDER = ""

# Name/folder of the project to build
if len(sys.argv) > 1:
    PROJECT_FOLDER = sys.argv[1]
if not os.path.exists(PROJECT_FOLDER):
    print("Please specify a valid project folder to build!")
    sys.exit(-1)

# Definitions (apk name, folders, etc.) will be taken from a json definition
if not os.path.isfile(os.path.join(PROJECT_FOLDER, "example.json")):
    print("Could not find json definition for example %s" % PROJECT_FOLDER)
    sys.exit(-1)

# Check if a build file is present, if not create one using the android SDK version specified
if not os.path.isfile(os.path.join(PROJECT_FOLDER, "build.xml")):
    print("Build.xml not present, generating with %s " % SDK_VERSION)
    ANDROID_CMD = "android"
    if os.name == 'nt':
        ANDROID_CMD += ".bat"
    if subprocess.call("%s update project -p ./%s -t %s" % (ANDROID_CMD, PROJECT_FOLDER, SDK_VERSION)) != 0:
        print("Error: Project update failed!")
        sys.exit(-1)

# Load example definition from json file
with open(os.path.join(PROJECT_FOLDER, "example.json")) as json_file:
    EXAMPLE_JSON = json.load(json_file)

APK_NAME = EXAMPLE_JSON["apkname"]
SHADER_DIR = EXAMPLE_JSON["directories"]["shaders"]

# Additional
ADDITIONAL_DIRS = []
ADDITIONAL_FILES = []
if "additional" in EXAMPLE_JSON["assets"]:
    ADDITIONAL = EXAMPLE_JSON["assets"]["additional"]
    if "directories" in ADDITIONAL:
        ADDITIONAL_DIRS = ADDITIONAL["directories"]
    if "files" in ADDITIONAL:
        ADDITIONAL_FILES = ADDITIONAL["files"]

# Get assets to be copied
ASSETS_MODELS = []
ASSETS_TEXTURES = []
if "assets" in EXAMPLE_JSON:
    ASSETS = EXAMPLE_JSON["assets"]
    if "models" in ASSETS:
        ASSETS_MODELS = EXAMPLE_JSON["assets"]["models"]
    if "textures" in ASSETS:
        ASSETS_TEXTURES = EXAMPLE_JSON["assets"]["textures"]

# Enable validation
VALIDATION = False
BUILD_ARGS = ""

for arg in sys.argv[1:]:
    if arg == "-validation":
        VALIDATION = True
        # Use a define to force validation in code
        BUILD_ARGS = "APP_CFLAGS=-D_VALIDATION"
        break

# Build
os.chdir(PROJECT_FOLDER)

if subprocess.call("ndk-build %s" %BUILD_ARGS, shell=True) == 0:
    print("Build successful")

    if VALIDATION:
        # Copy validation layers
        # todo: Currently only arm v7
        print("Validation enabled, copying validation layers...")
        os.makedirs("./libs/armeabi-v7a", exist_ok=True)
        for file in glob.glob("../layers/armeabi-v7a/*.so"):
            print("\t" + file)
            shutil.copy(file, "./libs/armeabi-v7a")

    # Create folders
    os.makedirs("./assets/shaders/base", exist_ok=True)
    os.makedirs("./assets/shaders/%s" % SHADER_DIR, exist_ok=True)
    os.makedirs("./assets/models", exist_ok=True)
    os.makedirs("./assets/textures", exist_ok=True)
    for directory in ADDITIONAL_DIRS:
        os.makedirs("./assets/%s" % directory, exist_ok=True)
    os.makedirs("./res/drawable", exist_ok=True)

    for filename in glob.glob("../../data/shaders/base/*.spv"):
        shutil.copy(filename, "./assets/shaders/base")
    for filename in glob.glob("../../data/shaders/%s/*.spv" %SHADER_DIR):
        shutil.copy(filename, "./assets/shaders/%s" % SHADER_DIR)
    for filename in ASSETS_MODELS:
        shutil.copy("../../data/models/%s" % filename, "./assets/models")
    for filename in ASSETS_TEXTURES:
        shutil.copy("../../data/textures/%s" % filename, "./assets/textures")
    for filename in ADDITIONAL_FILES:
        if "*.*" in filename:
            for fname in glob.glob("../../data/%s" % filename):
                locfname = fname.replace("../../data/", "")
                shutil.copy(fname, "./assets/%s" % locfname)
        else:
            shutil.copy("../../data/%s" % filename, "./assets/%s" % filename)

    shutil.copy("../../android/images/icon.png", "./res/drawable")

    if subprocess.call("ant debug -Dout.final.file=%s.apk" % APK_NAME, shell=True) == 0:
        # Deploy to device
        for arg in sys.argv[1:]:
            if arg == "-deploy":
                if subprocess.call("adb install -r %s.apk" % APK_NAME, shell=True) != 0:
                    print("Could not deploy to device!")
    else:
        print("Error during build process!")
        sys.exit(-1)
else:
    print("Error building project!")
    sys.exit(-1)

# Copy apk to bin folder
os.makedirs("../bin", exist_ok=True)
shutil.move('%s.apk' % APK_NAME, "../bin/%s.apk" % APK_NAME)
