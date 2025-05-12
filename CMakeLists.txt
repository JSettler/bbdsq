# [complete program & comments (except this one) are 100% done by Gemini 2.5 pro]

cmake_minimum_required(VERSION 3.10)
project(bbdsq VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17) # Or 14 if your GCC is older, but 17 is good for modern C++
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF) # Prefer to explicitly use std features


# --- Compiler Flags ---
# You can build with: cmake -DCMAKE_BUILD_TYPE=Debug ..
# Or: cmake -DCMAKE_BUILD_TYPE=Release ..
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra -Wpedantic -fsanitize=address,undefined")
# MODIFIED LINE BELOW: Added -flto
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG -flto -Wall -Wextra -Wpedantic") # -DNDEBUG disables asserts

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release) # << CHANGED HERE
  message(STATUS "No build type specified, defaulting to Release.")
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
# message(STATUS "C Compiler: ${CMAKE_C_COMPILER}") # Not using C compiler directly
message(STATUS "CXX Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "CXX Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CXX Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")


# --- Find SFML Package ---
# SFML 2.5+ provides CMake config files, which is the preferred way.
# For older SFML or if config files are not found, it might fall back to FindSFML.cmake.
find_package(SFML 2.5 COMPONENTS system window graphics REQUIRED)

# --- Add Source Files ---
# List all your .cpp files here. Headers are found via #include directives.
add_executable(${PROJECT_NAME}
    main.cpp
    bitboard.cpp
    piece.cpp
    movegen.cpp
    evaluation.cpp
    ai.cpp
    pst.cpp
    gui.cpp
    board_state_io.cpp
    zobrist.cpp
    ttable.cpp
)

# --- Link SFML Libraries ---
if(SFML_FOUND)
    # Using modern CMake approach: SFML::system, SFML::window, SFML::graphics
    # These targets usually carry their own include directories and dependencies.
    target_link_libraries(${PROJECT_NAME} PRIVATE sfml-system sfml-window sfml-graphics)
    message(STATUS "SFML Found. Version: ${SFML_VERSION_STRING}")
    # SFML_INCLUDE_DIR might not be set if using modern imported targets, as includes are transitive.
    # if(SFML_INCLUDE_DIR)
    #    message(STATUS "SFML Include Dirs: ${SFML_INCLUDE_DIR}")
    # endif()
else()
    message(FATAL_ERROR "SFML not found! Please install SFML development libraries (e.g., libsfml-dev).")
endif()

# --- Include Directories (Optional) ---
# If you decide to put your .h files in an 'include' subdirectory:
# target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
# For now, we assume .h files are in the same directory as .cpp files or standard include paths.
# CMake automatically adds the current source and binary directories to the include path.

# --- For sanitizers to work best with shared libraries (like SFML) ---
# This ensures that the main executable is also linked with sanitizer runtimes
# if the compiler flags for sanitizers are used.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_CXX_FLAGS MATCHES ".*fsanitize.*")
        # Check if flags already contain -fsanitize to avoid duplicate linking options
        # Some CMake versions/compilers handle this automatically with CMAKE_CXX_FLAGS.
        # Explicitly adding link options can sometimes be necessary.
        # However, simply setting CMAKE_CXX_FLAGS_DEBUG should be sufficient for GCC/Clang
        # to link the necessary runtime libraries for sanitizers.
        # If you encounter issues, you might add:
        # target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address,undefined)
        # But usually, it's handled by the compiler flags themselves.
        message(STATUS "Sanitizers enabled in CXX_FLAGS for Debug build.")
    endif()
endif()

# --- Output executable to root of build directory ---
# By default, it goes into build/CMakeFiles/project.dir/ or similar on some systems
# This makes it easier to find: build/bbdsq
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}) # For shared libs if any
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}) # For static libs if any


