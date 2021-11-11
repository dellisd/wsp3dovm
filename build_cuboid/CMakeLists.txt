set(PROJECT_NAME build_cuboid)

################################################################################
# Source groups
################################################################################
set(Source_Files
    "main.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES
    ${Source_Files}
)

################################################################################
# Target
################################################################################
add_executable(${PROJECT_NAME} ${ALL_FILES})

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
set(ROOT_NAMESPACE build_cuboid)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_GLOBAL_KEYWORD "Win32Proj"
)
set_target_properties(${PROJECT_NAME} PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE        "TRUE"
    INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO "TRUE"
)
################################################################################
# Include directories
################################################################################
target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/home/derek/projects/wsp3dovm/OpenVolumeMesh/install/include"
)

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
            /Oi;
            /Gy
        >
        $<$<CONFIG:RelWithDebInfo>:
            /O2;
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
            /INCREMENTAL
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
add_library(OpenVolumeMesh SHARED IMPORTED)
set_target_properties(OpenVolumeMesh PROPERTIES IMPORTED_LOCATION "/usr/local/lib/libOpenVolumeMesh.so")
set(ADDITIONAL_LIBRARY_DEPENDENCIES
    "$<$<CONFIG:Debug>:"
        "OpenVolumeMeshd"
    ">"
    "$<$<CONFIG:Release>:"
        "OpenVolumeMesh"
    ">"
    "$<$<CONFIG:RelWithDebInfo>:"
        "OpenVolumeMesh"
    ">"
    OpenVolumeMesh
)
target_link_libraries(${PROJECT_NAME} PRIVATE "${ADDITIONAL_LIBRARY_DEPENDENCIES}")

target_link_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}//OpenVolumeMesh/install/lib"
)

