# Single example build and deploy script
import os
import subprocess
import sys
import shutil
import glob

# Android SDK version used
SDK_VERSION = "android-23"

PROJECT_FOLDER = ""

# Name/folder of the project to build
if len(sys.argv) > 1:
    PROJECT_FOLDER = sys.argv[1]
if not os.path.exists(PROJECT_FOLDER):
    print("Please specify a valid project folder to build!")
    sys.exit(-1)

# Check if a build file is present, if not create one using the android SDK version specified
if not os.path.isfile(os.path.join(PROJECT_FOLDER, "build.xml")):
    print("Build.xml not present, generating with %s " % SDK_VERSION)
    if subprocess.call("android.bat update project -p ./%s -t %s" % (PROJECT_FOLDER, SDK_VERSION)) != 0:
        print("Error: Project update failed!")
        sys.exit(-1)

# Run actual build script from example folder
if not os.path.isfile(os.path.join(PROJECT_FOLDER, "build.py")):
    print("Error: No build script present!")
    sys.exit(-1)

BUILD_ARGUMENTS = " ".join(sys.argv[2:])

os.chdir(PROJECT_FOLDER)
if subprocess.call("python build.py %s" % BUILD_ARGUMENTS) != 0:
    print("Error during build process!")
    sys.exit(-1)

# Move apk to bin folder
os.makedirs("../bin", exist_ok=True)
for file in glob.glob("vulkan*.apk"):
    print(file)
    shutil.move(file, "../bin/%s" % file)
