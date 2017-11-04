#!/usr/bin/env python3

import os
import subprocess
import sys
import shutil
import glob
import json
import argparse

def main(project_folder, deploy=False, validation=False):

    # Android SDK version used
    SDK_VERSION = "android-23"

    # Check if python 3, python 2 not supported
    if sys.version_info <= (3, 0):
        print("Sorry, requires Python 3.x, not Python 2.x")
        return False

    # Definitions (apk name, folders, etc.) will be taken from a json definition
    if not os.path.isfile(os.path.join(project_folder, "example.json")):
        print("Could not find json definition for example %s" % project_folder)
        return False

    # Check if a build file is present, if not create one using the android SDK version specified
    if not os.path.isfile(os.path.join(project_folder, "build.xml")):
        print("Build.xml not present, generating with %s " % SDK_VERSION)
        ANDROID_CMD = "android"
        if os.name == 'nt':
            ANDROID_CMD += ".bat"
        if subprocess.call(("%s update project -p ./%s -t %s" % (ANDROID_CMD, project_folder, SDK_VERSION)).split(' ')) != 0:
            print("Error: Project update failed!")
            return False

    # Load example definition from json file
    with open(os.path.join(project_folder, "example.json")) as json_file:
        EXAMPLE_JSON = json.load(json_file)

    APK_NAME = EXAMPLE_JSON["apkname"]
    SHADER_DIR = EXAMPLE_JSON["directories"]["shaders"]

    # Additional
    ADDITIONAL_DIRS = []
    ADDITIONAL_FILES = []
    if "assets" in EXAMPLE_JSON and "additional" in EXAMPLE_JSON["assets"]:
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
    BUILD_ARGS = ""

    if validation:
        # Use a define to force validation in code
        BUILD_ARGS = "APP_CFLAGS=-D_VALIDATION"

    # Verify submodules are loaded in external folder
    if not os.listdir("../external/glm/") or not os.listdir("../external/gli/"):
        print("External submodules not loaded. Clone them using:")
        print("\tgit submodule init\n\tgit submodule update")
        return False

    # Build
    old_cwd = os.getcwd()
    os.chdir(project_folder)

    # Create android build files from templates
    os.makedirs("./jni", exist_ok=True)
    shutil.copy("./../templates/Application.mk", "./jni/")
    with open("./../templates/Android.mk", "rt") as fin:
        with open("./jni/Android.mk", "wt") as fout:
            for line in fin:
                fout.write(line.replace("%APK_NAME%", "%s" % APK_NAME).replace("%SRC_FOLDER%", "%s" % project_folder))

    if subprocess.call("ndk-build %s" %BUILD_ARGS, shell=True) == 0:
        print("Build successful")

        if validation:
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

        # Copy assets
        for filename in glob.glob("../../data/shaders/base/*.spv"):
            shutil.copy(filename, "./assets/shaders/base")
        for filename in glob.glob("../../data/shaders/%s/*.spv" % SHADER_DIR):
            shutil.copy(filename, "./assets/shaders/%s" % SHADER_DIR)
        for filename in ASSETS_MODELS:
            shutil.copy("../../data/models/%s" % filename, "./assets/models")
        for filename in ASSETS_TEXTURES:
            shutil.copy("../../data/textures/%s" % filename, "./assets/textures")
        for filename in ADDITIONAL_FILES:
            if "*." in filename:
                for fname in glob.glob("../../data/%s" % filename):
                    locfname = fname.replace("../../data/", "")
                    shutil.copy(fname, "./assets/%s" % locfname)
            else:
                shutil.copy("../../data/%s" % filename, "./assets/%s" % filename)

        shutil.copy("../../android/images/icon.png", "./res/drawable")

        if subprocess.call("ant debug -Dout.final.file=%s.apk" % APK_NAME, shell=True) == 0:
            if deploy and subprocess.call("adb install -r %s.apk" % APK_NAME, shell=True) != 0:
                print("Could not deploy to device!")
        else:
            print("Error during build process!")
            return False
    else:
        print("Error building project!")
        return False

    # Copy apk to bin folder
    os.makedirs("../bin", exist_ok=True)
    shutil.move('%s.apk' % APK_NAME, "../bin/%s.apk" % APK_NAME)
    os.chdir(old_cwd)
    return True

class ReadableDir(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        if not os.path.isdir(values):
            raise argparse.ArgumentTypeError("{0} is not a valid path".format(values))
        if not os.access(values, os.R_OK):
            raise argparse.ArgumentTypeError("{0} is not a readable dir".format(values))
        setattr(namespace, self.dest,values)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Build and deploy a single example")
    parser.add_argument('-deploy', default=False, action='store_true', help="install example on device")
    parser.add_argument('-validation', default=False, action='store_true')
    parser.add_argument('project_folder', action=ReadableDir)
    try:
        args = parser.parse_args()
    except SystemExit:
        sys.exit(1)
    ok = main(args.project_folder, args.deploy, args.validation)
    sys.exit(0 if ok else 1)
