cmake_minimum_required (VERSION 3.10)

file (STRINGS version VERSION_RAW)
string (STRIP "${VERSION_RAW}" VERSION_STRIPPED)
string (REGEX REPLACE "-dev$" "" VERSION "${VERSION_STRIPPED}")

project (netpbm2osbadgeicons
  DESCRIPTION "Convert Netpbm images to OS-BADGE-ICONS information"
  VERSION ${VERSION}
  LANGUAGES C
)

# Set allowed values for CMAKE_BUILD_TYPE
set (ALLOWED_BUILD_TYPES "Debug;RelWithDebInfo;Release")
set (CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING "${ALLOWED_BUILD_TYPES}" FORCE)
set_property (CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "${ALLOWED_BUILD_TYPES}")

# Check CMAKE_BUILD_TYPE
string (TOUPPER "${ALLOWED_BUILD_TYPES}" ALLOWED_BUILD_TYPES_UPPER)
string (TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_UPPER)
string (FIND "${ALLOWED_BUILD_TYPES_UPPER}" "${CMAKE_BUILD_TYPE_UPPER}" BUILDTYPE_POS_IN_ALLOWLIST)
if (BUILDTYPE_POS_IN_ALLOWLIST LESS 0)
  message (FATAL_ERROR "CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} not in list of allowed types: ${ALLOWED_BUILD_TYPES}")
endif()

set (CMAKE_C_STANDARD 99)
set (CMAKE_C_EXTENSIONS OFF)
set (CMAKE_C_STANDARD_REQUIRED ON)

include (CTest)
include (GNUInstallDirs)

set (PRIVATE_COMPILE_OPTIONS)

if (MSVC)
  list (APPEND PRIVATE_COMPILE_OPTIONS "/W4")
else()
  list (APPEND PRIVATE_COMPILE_OPTIONS
    "-Wall"
    "-Wextra"
    "-pedantic"
  )
endif()

option (NETPBM2OSBADGEICONS_WERROR "Treat warnings as errors" OFF)
if (NETPBM2OSBADGEICONS_WERROR)
  if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    set (CMAKE_COMPILE_WARNING_AS_ERROR ON)
  elseif (MSVC)
    list (APPEND PRIVATE_COMPILE_OPTIONS "/WX")
  else()
    list (APPEND PRIVATE_COMPILE_OPTIONS "-Werror")
  endif()

  # don't care about *_s functions on MSVC
  if (MSVC)
    list (APPEND PRIVATE_COMPILE_OPTIONS "/D_CRT_SECURE_NO_WARNINGS")
  endif()

  # unused function parameter in placeholder function
  if (MSVC)
    list (APPEND PRIVATE_COMPILE_OPTIONS "/wd4100")
  else()
    list (APPEND PRIVATE_COMPILE_OPTIONS "-Wno-error=unused-parameter")
  endif()

endif()

# TODO: Check if function available even when libm not found / unavailable
find_library (LIBM m) # round()

add_executable ("${PROJECT_NAME}" netpbm2osbadgeicons.c)

target_compile_options ("${PROJECT_NAME}"
  PRIVATE "${PRIVATE_COMPILE_OPTIONS}"
)

if (LIBM)
  target_link_libraries ("${PROJECT_NAME}"
    PRIVATE "${LIBM}"
  )
endif()

install (TARGETS "${PROJECT_NAME}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

if (BUILD_TESTING)
  add_subdirectory (tests)
endif()
