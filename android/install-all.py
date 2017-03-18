# Install all examples to connected device(s)
import subprocess
import sys

answer = input("Install all vulkan examples to attached device, this may take some time! (Y/N)").lower() == 'y'
if answer:
    BUILD_ARGUMENTS = ""
    for arg in sys.argv[1:]:
        if arg == "-validation":
            BUILD_ARGUMENTS += "-validation"
    if subprocess.call("python build-all.py -deploy %s" % BUILD_ARGUMENTS) != 0:
        print("Error: Not all examples may have been installed!")
        sys.exit(-1)
    