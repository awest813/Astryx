#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Boost::locale" for configuration "Release"
set_property(TARGET Boost::locale APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Boost::locale PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/boost_locale-vc145-mt-x64-1_90.lib"
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELEASE "Boost::charconv;Boost::thread"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/boost_locale-vc145-mt-x64-1_90.dll"
  )

list(APPEND _cmake_import_check_targets Boost::locale )
list(APPEND _cmake_import_check_files_for_Boost::locale "${_IMPORT_PREFIX}/lib/boost_locale-vc145-mt-x64-1_90.lib" "${_IMPORT_PREFIX}/bin/boost_locale-vc145-mt-x64-1_90.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
