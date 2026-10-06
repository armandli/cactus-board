# Imports the prebuilt Cactus Needle 3 engine from needle3/ as needle::needle.
# The files are downloaded by `make needle` (see needle3/README.md).

set(NEEDLE_DIR "${PROJECT_SOURCE_DIR}/needle3")
set(NEEDLE_MODEL_PATH "${NEEDLE_DIR}/needle3.cact" CACHE FILEPATH "Default Needle 3 weights (.cact)")

foreach(f needle.h libneedle.a)
  if(NOT EXISTS "${NEEDLE_DIR}/${f}")
    message(FATAL_ERROR "Missing ${NEEDLE_DIR}/${f}. Run `make needle` to download the Needle 3 engine.")
  endif()
endforeach()

add_library(needle::needle STATIC IMPORTED GLOBAL)
set_target_properties(needle::needle PROPERTIES
  IMPORTED_LOCATION "${NEEDLE_DIR}/libneedle.a"
  INTERFACE_INCLUDE_DIRECTORIES "${NEEDLE_DIR}"
)

# libneedle.a is built with libc++ (Clang); on Linux link its runtime.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set_property(TARGET needle::needle APPEND PROPERTY
    INTERFACE_LINK_LIBRARIES c++ c++abi
  )
endif()
