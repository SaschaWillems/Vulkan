# Copyright (C) 2025 by Sascha Willems - www.saschawillems.de
# This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)

# Runs all samples in benchmark mode and stores all validation messages into a single text file

# Note: Need to be copied to where the binary files have been compiled

import glob
import subprocess
import os

if os.path.exists("validation_output.txt"):
  os.remove("validation_output.txt")
i = 0
for sample in glob.glob("*.exe"):
    # Skip headless samples, as they require manual keypress
    if "headless" in sample:
       continue
    subprocess.call("%s -v -vl -b -bfs %s" % (sample, 50), shell=True)