#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "unofficial::stackwalker::StackWalker" for configuration "Debug"
set_property(TARGET unofficial::stackwalker::StackWalker APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(unofficial::stackwalker::StackWalker PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/StackWalker.lib"
  )

list(APPEND _cmake_import_check_targets unofficial::stackwalker::StackWalker )
list(APPEND _cmake_import_check_files_for_unofficial::stackwalker::StackWalker "${_IMPORT_PREFIX}/debug/lib/StackWalker.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
