file(GLOB COMMON_SOURCE *.cpp *.h *.hpp)
add_library(base ${COMMON_SOURCE})
set_target_properties(base PROPERTIES FOLDER "common")