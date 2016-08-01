import sys
import os
import glob
import subprocess

if len(sys.argv) < 2:
	sys.exit("Please provide a target directory")

if not os.path.exists(sys.argv[1]):
	sys.exit("%s is not a valid directory" % sys.argv[1])

path = sys.argv[1]

shaderfiles = []
for exts in ('*.vert', '*.frag', '*.comp', '*.geom', '*.tesc', '*.tese'):
	shaderfiles.extend(glob.glob(os.path.join(path, exts)))

failedshaders = []
for shaderfile in shaderfiles:
		print("\n-------- %s --------\n" % shaderfile)
		if subprocess.call("glslangvalidator -V %s -o %s.spv" % (shaderfile, shaderfile), shell=True) != 0:
			failedshaders.append(shaderfile)

print("\n-------- Compilation result --------\n")

if len(failedshaders) == 0:
	print("SUCCESS: All shaders compiled to SPIR-V")
else:
	print("ERROR: %d shader(s) could not be compiled:\n" % len(failedshaders))
	for failedshader in failedshaders:
		print("\t" + failedshader)
