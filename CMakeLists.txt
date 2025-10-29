cmake_minimum_required(VERSION 3.18)

if(APPLE)
    project(Tesi LANGUAGES CXX OBJCXX)
else()
    project(Tesi LANGUAGES CXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCL REQUIRED)

add_compile_definitions(FF_HEADER_ONLY NO_DEFAULT_MAPPING)

# ========== Clonazione di FastFlow da GitHub =======
set(FASTFLOW_DIR "${CMAKE_SOURCE_DIR}/external/fastflow")

if(NOT EXISTS ${FASTFLOW_DIR})
    message(STATUS "FastFlow not found. Cloning from GitHub...")
    file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/external")
    execute_process(
        COMMAND git clone --depth 1 https://github.com/fastflow/fastflow.git ${FASTFLOW_DIR}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/external
        RESULT_VARIABLE GIT_RESULT
        OUTPUT_VARIABLE GIT_OUTPUT
        ERROR_VARIABLE GIT_ERROR
    )
    if(NOT GIT_RESULT EQUAL 0)
        message(FATAL_ERROR "Errore during the clonation of FastFlow: ${GIT_ERROR}")
    else()
        message(STATUS "FastFlow cloned successfully in ${FASTFLOW_DIR}")
    endif()
else()
    message(STATUS "Found FastFlow: ${FASTFLOW_DIR}")
endif()
# ===================================================

# Lista dei file sorgente comuni a tutte le piattaforme.
set(COMMON_SOURCES
    src/main.cpp
    src/ff_Pipe_nodes/ff_node_acc_t.cpp
    src/cpu_runner/Cpu_FF_Runner.cpp
    src/accelerator/BufferManager.cpp
    src/helpers/Helpers.cpp
)

# Aggiunge i file sorgente e le librerie specifiche per ogni piattaforma.
if(APPLE)
    list(APPEND COMMON_SOURCES 
        src/accelerator/Gpu_OpenCL_Accelerator.cpp
        src/accelerator/Gpu_Metal_Accelerator.mm
    )
    find_library(METAL_LIBRARY Metal REQUIRED)
    find_library(FOUNDATION_LIBRARY Foundation REQUIRED)
    set(PLATFORM_LIBS ${METAL_LIBRARY} ${FOUNDATION_LIBRARY})
else()
    list(APPEND COMMON_SOURCES 
        src/cpu_runner/Cpu_OMP_Runner.cpp
        src/accelerator/FpgaAccelerator.cpp
    )
    set(PLATFORM_LIBS stdc++fs)
endif()

add_executable(tesi-exec ${COMMON_SOURCES})

# Specifica le directory dove il compilatore deve cercare gli .hpp
target_include_directories(tesi-exec PRIVATE
    SYSTEM ${CMAKE_SOURCE_DIR}/external/fastflow
    ${CMAKE_SOURCE_DIR}/include
    ${OpenCL_INCLUDE_DIRS})

# Linka le librerie comuni e quelle specifiche della piattaforma.
target_link_libraries(tesi-exec PRIVATE 
    OpenCL::OpenCL
    ${PLATFORM_LIBS}
)

# Aggiunge e linka OpenMP solo su sistemi non-Apple (Linux).
if(NOT APPLE)
    find_package(OpenMP)
    if(OpenMP_FOUND)
        target_link_libraries(tesi-exec PRIVATE OpenMP::OpenMP_CXX)
    endif()
    target_link_libraries(tesi-exec PRIVATE "-static-libstdc++")
endif()
    
target_compile_definitions(tesi-exec PRIVATE CL_TARGET_OPENCL_VERSION=120)
target_compile_options(tesi-exec PRIVATE -Wno-deprecated-declarations)

# Diciamo a CMake di trattare il file .mm come Objective-C++ e di attivare ARC.
if(APPLE)
    set_source_files_properties(src/accelerator/Gpu_Metal_Accelerator.mm PROPERTIES
        LANGUAGE OBJCXX
        COMPILE_FLAGS "-fobjc-arc"
    )
endif()