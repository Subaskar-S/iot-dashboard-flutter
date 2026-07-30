# IoT Dashboard C++20 Compilation Flags
# Production-ready compiler flags for C++20 applications

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "^(AppleClang|Clang|GNU)$")
  # Enable warnings
  add_flag(-Wall)
  add_flag(-Wextra)
  add_flag(-Wpedantic)
  add_flag(-Woverloaded-virtual)     # warn if you overload (not override) a virtual function
  add_flag(-Wformat=2)               # warn on security issues around functions that format output
  add_flag(-Wmisleading-indentation) # warn if indentation implies blocks where blocks do not exist
  add_flag(-Wduplicated-cond)        # warn if if / else chain has duplicated conditions
  add_flag(-Wduplicated-branches)    # warn if if / else branches have duplicated code
  add_flag(-Wnull-dereference)       # warn if a null dereference is detected
  add_flag(-Wsign-compare)
  add_flag(-Wtype-limits)
  add_flag(-Wnon-virtual-dtor)       # warn if class with virtual functions has non-virtual destructor

  # C++20 specific warnings
  add_flag(-Wc++20-compat)           # warn about C++20 compatibility issues
  
  # Disable noisy warnings
  add_flag(-Wno-unused-command-line-argument)
  add_flag(-Wno-unused-variable)
  add_flag(-Wno-unused-parameter)
  add_flag(-Wno-unused-function)
  add_flag(-Wno-format-nonliteral)
  add_flag(-Wno-gnu-zero-variadic-macro-arguments)

  # Performance
  add_flag(-march=native)            # optimize for current CPU
endif()

# MSVC flags
if (MSVC)
  add_flag(/W4)                      # Warning level 4
  add_flag(/std:c++20)               # C++20 standard
  add_flag(/permissive-)             # Standards conformance mode
  add_flag(/Zc:__cplusplus)          # Enable __cplusplus macro
  add_flag(/EHsc)                    # Exception handling
endif()
