file(GLOB BASE_SRC *.cpp)
file(GLOB BASE_HEADERS *.hpp)

add_library(base SHARED ${BASE_SRC})
