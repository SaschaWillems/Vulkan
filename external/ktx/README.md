<img src="https://www.khronos.org/assets/images/api_logos/khronos.svg" width="300"/>

The Official Khronos KTX Software Repository
---

| GNU/Linux, iOS & OSX |  Windows | Documentation | 
|----------------------| :------: | :-----------: |
| [![Build Status](https://travis-ci.org/KhronosGroup/KTX-Software.svg?branch=master)](https://travis-ci.org/KhronosGroup/KTX-Software) | [![Build status](https://ci.appveyor.com/api/projects/status/rj9bg8g2jphg3rc0/branch/master?svg=true)](https://ci.appveyor.com/project/msc-/ktx/branch/master) | [![Build status](https://codedocs.xyz/KhronosGroup/KTX-Software.svg)](https://codedocs.xyz/KhronosGroup/KTX-Software/) |

This is the official home of the source code
for the Khronos KTX library and tools. KTX is a lightweight file format
for OpenGL textures, designed around how textures are loaded in OpenGL.

See the Doxygen generated live documentation for
[`master`](https://github.khronos.org/KTX-Software/)
for API usage information.

See [CONTRIBUTING](CONTRIBUTING.md) for information about contributing.

See [LICENSE](LICENSE.md) for information about licensing.

See [BUILDING](BUILDING.md) for information about building the code.

More information about KTX and links to tools that support it can be
found on the
[KTX page](http://www.khronos.org/opengles/sdk/tools/KTX/) of
the [OpenGL ES SDK](http://www.khronos.org/opengles/sdk) on
[khronos.org](http://www.khronos.org).

If you need help with using the KTX library or KTX tools, please use the
[KTX forum](https://forums.khronos.org/forumdisplay.php/103-KTX-file-format-for-OpenGL-OpenGL-ES-and-WebGL-textures).
To report problems use GitHub [issues](https://github.com/KhronosGroup/KTX/issues).

**IMPORTANT:** you **must** install the [Git LFS](https://github.com/github/git-lfs)
command line extension in order to fully checkout this repository after cloning. You
need at least version 1.1.

A few files have `$Date$` keywords. If you care about having the proper
dates shown or will be generating the documentation or preparing
distribution archives, you **must** follow the instructions below.

#### <a id="kwexpansion"></a>$Date$ keyword expansion

$Date$ keywords are expanded via a smudge & clean filter. To install
the filter, issue the following commands in the root of your clone.

On Unix (Linux, Mac OS X, etc.) platforms and Windows using Git for Windows'
Git Bash or Cygwin's bash terminal:

```bash
./install-gitconfig.sh
rm TODO.md include/ktx.h tools/toktx/toktx.cpp
git checkout TODO.md include/ktx.h tools/toktx/toktx.cpp
```

On Windows with the Command Prompt (requires `git.exe` in a directory
on your %PATH%):

```cmd
install-gitconfig.bat
del TODO.md include/ktx.h tools/toktx/toktx.cpp
git checkout TODO.md include/ktx.h tools/toktx/toktx.cpp 
```

The first command adds an [include] of the repo's `.gitconfig` to the
local git config file `.git/config`, i.e. the one in your clone of the repo.
`.gitconfig` contains the config of the "keyworder" filter. The remaining
commands force a new checkout of the affected files to smudge them with the
date. These two are unnecessary if you plan to edit these files.

