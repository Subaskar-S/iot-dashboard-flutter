# CMake helper functions for IoT Dashboard project

# Add a compiler flag if supported
function(add_flag flag)
  string(REGEX REPLACE "[^a-zA-Z0-9]" "_" flag_var ${flag})
  set(flag_var "FLAG${flag_var}")
  
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag(${flag} ${flag_var})
  
  if(${${flag_var}})
    add_compile_options(${flag})
  endif()
endfunction()

# Print a CMake variable for debugging
function(print)
  message(STATUS "${ARGN}")
endfunction()

# Get the workspace root directory (parent of current PROJECT_SOURCE_DIR)
function(get_default_root result)
  get_filename_component(_root "${PROJECT_SOURCE_DIR}/.." ABSOLUTE)
  set(${result} "${_root}" PARENT_SCOPE)
endfunction()
