set(PROJECT_NAME wsp3dovm)

################################################################################
# Source groups
################################################################################
set(no_group_source_files
    "ClassDiagram.cd"
)
source_group("" FILES ${no_group_source_files})

set(Header_Files
    "common.h"
    "create_steinerpoints.h"
    "MeshGenerator.hpp"
    "read_tet.h"
    "statistics.h"
    "write_tet.h"
    "write_vtk.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Source_Files
    "create_steinerpoints.cpp"
    "main.cpp"
    "read_tet.cpp"
    "statistics.cpp"
    "write_tet.cpp"
    "write_vtk.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES
    ${no_group_source_files}
    ${Header_Files}
    ${Source_Files}
)

################################################################################
# Target
################################################################################
add_executable(${PROJECT_NAME} ${ALL_FILES})

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
set(ROOT_NAMESPACE wsd3dovm)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_GLOBAL_KEYWORD "Win32Proj"
)
set_target_properties(${PROJECT_NAME} PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE        "TRUE"
    INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO "TRUE"
)
################################################################################
# MSVC runtime library
################################################################################
get_property(MSVC_RUNTIME_LIBRARY_DEFAULT TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY)
string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
    $<$<CONFIG:Debug>:
        MultiThreadedDebugDLL
    >
    $<$<NOT:$<OR:$<CONFIG:Debug>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
)
set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})

################################################################################
# Include directories
################################################################################
if(NOT EMSCRIPTEN)
    find_package(Boost 1.70 COMPONENTS filesystem program_options chrono REQUIRED)

    target_include_directories(${PROJECT_NAME} PUBLIC
        "${Boost_INCLUDE_DIR}"
    )
endif()
set(Boost_USE_STATIC_LIBS ON)

################################################################################
# Compile definitions
################################################################################
target_compile_definitions(${PROJECT_NAME} PRIVATE
    "$<$<CONFIG:Debug>:"
        "_DEBUG"
    ">"
    "$<$<CONFIG:Release>:"
        "NDEBUG"
    ">"
    "$<$<CONFIG:RelWithDebInfo>:"
        "NDEBUG"
    ">"
    "_CONSOLE;"
    "_LIB;"
    "UNICODE;"
    "_UNICODE"
)

################################################################################
# Compile and link options
################################################################################
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
            /Od
        >
        $<$<CONFIG:Release>:
            /O2;
            /Ob2;
            /Oi;
            /arch:AVX2;
            /Gy
        >
        $<$<CONFIG:RelWithDebInfo>:
            /O2;
            /Ob2;
            /Oi;
            /Gy
        >
        /sdl;
        /W3;
        ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
        ${DEFAULT_CXX_EXCEPTION_HANDLING};
        /Y-
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
            /INCREMENTAL;
            /VERBOSE
        >
        $<$<CONFIG:Release>:
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO
        >
        $<$<CONFIG:RelWithDebInfo>:
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO
        >
        /DEBUG;
        /SUBSYSTEM:CONSOLE
    )
endif()

################################################################################
# Dependencies
################################################################################
if(EMSCRIPTEN)
    set_target_properties(OpenVolumeMesh PROPERTIES IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/_deps/ovm-build/src/libOpenVolumeMesh.a")
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        OpenVolumeMesh
    )
else()
    set_target_properties(OpenVolumeMesh PROPERTIES IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/_deps/ovm-build/src/libOpenVolumeMesh.so")
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        Boost::filesystem
        Boost::program_options
        Boost::chrono
        OpenVolumeMesh
    )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE "${ADDITIONAL_LIBRARY_DEPENDENCIES}")
if(EMSCRIPTEN)
    set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-s USE_BOOST_HEADERS=1" LINK_FLAGS "-s USE_BOOST_HEADERS=1 -s EXPORT_ES6=1 -s MODULARIZE=1 -s --bind")
endif()

