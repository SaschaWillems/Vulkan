# Find assimp
# Copyright © 2016 Dylan Baker

# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
# OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

# Provides a subset of what the one would get by calling CONFIG REQUIRED
# directly, it will unset any unsupported versions
# Currently it provides the following:
#  ASSIMP_FOUND      -- TRUE if assimp was found
#  ASSIMP_LIBRARIES  -- Libraries to link against

find_package(ASSIMP CONFIG)
if (ASSIMP_LIBRARIES)
    # Unset variables that would be difficult to get via the find module
    unset(ASSIMP_ROOT_DIR)
    unset(ASSIMP_CXX_FLAGS)
    unset(ASSIMP_LINK_FLAGS)
    unset(ASSIMP_Boost_VERSION)

    # TODO: It would be nice to find these, but they're not being used at the
    # moment. They also need to be added to REQUIRED_VARS if added
    unset(ASSIMP_INCLUDE_DIRS)
    unset(ASSIMP_LIBRARY_DIRS)

    # Like the found path
    set(ASSIMP_FOUND TRUE)
    message("-- Found ASSIMP")
else ()
    find_library(ASSIMP_LIBRARIES
                 NAMES assimp
    )

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(
        ASSIMP
        REQUIRED_VARS ASSIMP_LIBRARIES
    )
endif (ASSIMP_LIBRARIES)

set(ASSIMP_FOUND True)
