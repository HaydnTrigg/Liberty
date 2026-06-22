# CMake minimum version
cmake_minimum_required(VERSION 4.2)

# Set the project name (change as needed)
project(Liberty LANGUAGES C CXX ASM_MASM CSharp)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Specify the NuGet executable URL
set(NUGET_URL "https://dist.nuget.org/win-x86-commandline/latest/nuget.exe")
set(NUGET_EXECUTABLE "${CMAKE_BINARY_DIR}/nuget.exe")

# I've got a lovely bunch of coconuts
if(NOT CMAKE_VS_PLATFORM_TOOLSET STREQUAL "ClangCL")
    message(FATAL_ERROR "Toolset must use ClangCL CMAKE_VS_PLATFORM_TOOLSET=${CMAKE_VS_PLATFORM_TOOLSET}" )
endif()

# There they are, all standing in a row
if(NOT CMAKE_VS_PLATFORM_NAME STREQUAL "Win32")
    message(FATAL_ERROR "Platform must be Win32 CMAKE_VS_PLATFORM_NAME=${CMAKE_VS_PLATFORM_NAME}" )
endif()

# Global output directories for binaries and libraries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_PDB_OUTPUT_DIRECTORY  "${CMAKE_BINARY_DIR}/pdb")

set(CMAKE_PDB_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug/pdb")
set(CMAKE_PDB_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/pdb")
set(CMAKE_PDB_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/MinSizeRel/pdb")
set(CMAKE_PDB_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/RelWithDebInfo/pdb")

set(LIBERTY_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
set(LIBERTY_EXE_DIRECTORY "Exe")
set(LIBERTY_TOOLS_DIRECTORY "Tools")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${LIBERTY_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${LIBERTY_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${LIBERTY_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${LIBERTY_DIRECTORY})

set(LIBERTY_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Code")
set(LIBERTY_GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Generated")

function(create_source_groups base_dir sources)
    foreach(file ${sources})
        # Get the relative path of the file with respect to the base directory
        file(RELATIVE_PATH file_relative "${base_dir}" "${file}")

        # Extract the directory part of the relative path
        get_filename_component(file_directory "${file_relative}" DIRECTORY)

        # Use the directory as the group name, or fall back to no group if empty
        if(file_directory)
            string(REPLACE "/" "\\" file_group "${file_directory}")
            source_group("${file_group}" FILES "${file}")
        else()
            source_group("" FILES "${file}")
        endif()
    endforeach()
endfunction()

function(generate_generated_sources TARGET_NAME TARGET_TYPE TARGET_FOLDERS)
    # Define a list of relative paths to exclude (adjust these paths as needed)
    set(EXCLUDED_FILES
        "tag_files/tag_definition_generator.cs")

    # Clear the list of generated sources and dependencies
    set(GENERATED_SOURCES "")
    set(GENERATED_DEPENDS "")

    foreach(TARGET_FOLDER IN LISTS TARGET_FOLDERS)
        file(GLOB_RECURSE TARGET_FILES "${LIBERTY_SOURCE_DIR}/${TARGET_FOLDER}/*.cs")
        foreach(file ${TARGET_FILES})
            file(READ ${file} fileContent)
            string(FIND "${fileContent}" "namespace Liberty.Definitions" foundPos)
            string(FIND "${fileContent}" "Export" foundPos2)
            if(NOT foundPos EQUAL -1)
                if(NOT foundPos2 EQUAL -1)
                    file(RELATIVE_PATH rel ${LIBERTY_SOURCE_DIR} ${file})
                    # Check if the relative path is NOT in the excluded list
                    if(NOT rel IN_LIST EXCLUDED_FILES)
                        cmake_path(REPLACE_EXTENSION rel LAST_ONLY ".h")
                        cmake_path(APPEND gen ${LIBERTY_GENERATED_DIR} ${rel})
                        list(APPEND GENERATED_SOURCES ${gen})
                        list(APPEND GENERATED_DEPENDS ${file})
                    endif()
                endif()
            endif()
        endforeach()
    endforeach()

    set(GENERATED_DEPENDS ${GENERATED_DEPENDS} PARENT_SCOPE)
    set(GENERATED_SOURCES ${GENERATED_SOURCES} PARENT_SCOPE)
endfunction()

function(add_liberty_project TARGET_NAME TARGET_TYPE TARGET_FOLDERS TARGET_SUBDIRECTORY)
    string(TOUPPER "${TARGET_NAME}" TARGET_NAME_UPPER)
    
    set(SOURCES "")
    set(CONFIG_SOURCES "")
    if(TARGET_FOLDERS)
        foreach(TARGET_FOLDER IN LISTS TARGET_FOLDERS)
            file(GLOB_RECURSE TARGET_FILES CONFIGURE_DEPENDS
                "${LIBERTY_SOURCE_DIR}/${TARGET_FOLDER}/**.hpp"
                "${LIBERTY_SOURCE_DIR}/${TARGET_FOLDER}/**.cpp"
                "${LIBERTY_SOURCE_DIR}/${TARGET_FOLDER}/**.h"
                "${LIBERTY_SOURCE_DIR}/${TARGET_FOLDER}/**.c"
                "${LIBERTY_SOURCE_DIR}/${TARGET_FOLDER}/**.inl"
                "${LIBERTY_SOURCE_DIR}/${TARGET_FOLDER}/**.rc")
            list(APPEND SOURCES ${TARGET_FILES})
            file(GLOB_RECURSE TARGET_FILES CONFIGURE_DEPENDS
                "${LIBERTY_SOURCE_DIR}/${TARGET_FOLDER}/**.ini"
                "${LIBERTY_SOURCE_DIR}/${TARGET_FOLDER}/**.fl")
            list(APPEND CONFIG_SOURCES ${TARGET_FILES})
        endforeach()
    else()
        file(GLOB_RECURSE TARGET_FILES CONFIGURE_DEPENDS
            "${LIBERTY_SOURCE_DIR}/**.hpp"
            "${LIBERTY_SOURCE_DIR}/**.cpp"
            "${LIBERTY_SOURCE_DIR}/**.h"
            "${LIBERTY_SOURCE_DIR}/**.c"
            "${LIBERTY_SOURCE_DIR}/**.inl"
            "${LIBERTY_SOURCE_DIR}/**.rc")
        list(APPEND SOURCES ${TARGET_FILES})
        file(GLOB_RECURSE TARGET_FILES CONFIGURE_DEPENDS
            "${LIBERTY_SOURCE_DIR}/**.ini"
            "${LIBERTY_SOURCE_DIR}/**.fl")
        list(APPEND CONFIG_SOURCES ${TARGET_FILES})
    endif()
    
    set(GENERATED_SOURCES "")
    set(GENERATED_DEPENDS "")
    
    set(IS_COMPILABLE False)
    if(TARGET_TYPE STREQUAL "library")
        add_library(${TARGET_NAME} STATIC ${SOURCES} ${CONFIG_SOURCES} ${GENERATED_SOURCES})
        target_compile_definitions(${TARGET_NAME} PUBLIC "${TARGET_NAME_UPPER}_DEC=")
        set(IS_COMPILABLE True)
    elseif(TARGET_TYPE STREQUAL "shared")
        add_library(${TARGET_NAME} SHARED ${SOURCES} ${CONFIG_SOURCES} ${GENERATED_SOURCES})
        target_compile_definitions(${TARGET_NAME} INTERFACE "${TARGET_NAME_UPPER}_DEC=__declspec(dllimport)")
        target_compile_definitions(${TARGET_NAME} PRIVATE "${TARGET_NAME_UPPER}_DEC=__declspec(dllexport)")
        set(IS_COMPILABLE True)
    elseif(TARGET_TYPE STREQUAL "resource")
        add_library(${TARGET_NAME} SHARED ${SOURCES} ${CONFIG_SOURCES} ${GENERATED_SOURCES})
        target_compile_definitions(${TARGET_NAME} INTERFACE "${TARGET_NAME_UPPER}_DEC=__declspec(dllimport)")
        target_compile_definitions(${TARGET_NAME} PRIVATE "${TARGET_NAME_UPPER}_DEC=__declspec(dllexport)")
        set(IS_COMPILABLE True)
        if(MSVC)
          set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS "/NOENTRY")
        endif()
    elseif(TARGET_TYPE STREQUAL "executable")
        add_executable(${TARGET_NAME} ${SOURCES} ${CONFIG_SOURCES} ${GENERATED_SOURCES})
        set_target_properties(${TARGET_NAME} PROPERTIES WIN32_EXECUTABLE ON)
        target_compile_definitions(${TARGET_NAME} INTERFACE "${TARGET_NAME_UPPER}_DEC=__declspec(dllimport)")
        target_compile_definitions(${TARGET_NAME} PRIVATE "${TARGET_NAME_UPPER}_DEC=__declspec(dllexport)")
        set(IS_COMPILABLE True)
    elseif(TARGET_TYPE STREQUAL "console")
        add_executable(${TARGET_NAME} ${SOURCES} ${CONFIG_SOURCES} ${GENERATED_SOURCES})
        target_compile_definitions(${TARGET_NAME} INTERFACE "${TARGET_NAME_UPPER}_DEC=__declspec(dllimport)")
        target_compile_definitions(${TARGET_NAME} PRIVATE "${TARGET_NAME_UPPER}_DEC=__declspec(dllexport)")
        set(IS_COMPILABLE True)
    else(TARGET_TYPE STREQUAL "noop")
        add_custom_target(${TARGET_NAME} SOURCES ${SOURCES} ${CONFIG_SOURCES} ${GENERATED_SOURCES})
    endif()

    if(${IS_COMPILABLE})
        set(PCH_FILE "${LIBERTY_SOURCE_DIR}/${TARGET_NAME}/${TARGET_NAME}_headers.cpp")
        if(EXISTS "${PCH_FILE}")
            target_precompile_headers(${TARGET_NAME} PRIVATE "${PCH_FILE}")
        endif()
        
        set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${LIBERTY_DIRECTORY}/${TARGET_SUBDIRECTORY}")
        set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${LIBERTY_DIRECTORY}/${TARGET_SUBDIRECTORY}")
        set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${LIBERTY_DIRECTORY}/${TARGET_SUBDIRECTORY}")
        set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${LIBERTY_DIRECTORY}/${TARGET_SUBDIRECTORY}")
        set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${LIBERTY_DIRECTORY}/${TARGET_SUBDIRECTORY}")

        foreach(CONFIG_FILE IN LISTS CONFIG_SOURCES)
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CONFIG_FILE}" "$<TARGET_FILE_DIR:${TARGET_NAME}>")
        endforeach()

        if (CMAKE_GENERATOR MATCHES "Visual Studio")
            target_compile_options(${TARGET_NAME} PRIVATE /MP)
            target_compile_options(${TARGET_NAME} PRIVATE /clang:-Wno-unused-private-field)
            target_compile_options(${TARGET_NAME} PRIVATE /clang:-ferror-limit=1000)
        endif()
        if(CMAKE_VS_PLATFORM_NAME STREQUAL "Win32")
            target_link_directories(${TARGET_NAME} PRIVATE "${DXSDK_LIB_DIR}/x86")
            target_include_directories(${TARGET_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Code/RP8/DirectX")
        elseif(CMAKE_VS_PLATFORM_NAME STREQUAL "x64")
            target_link_directories(${TARGET_NAME} PRIVATE "${DXSDK_LIB_DIR}/x64")
        else()
            message(FATAL_ERROR "Unsupported DXSDK platform '${CMAKE_VS_PLATFORM_NAME}'")
        endif()

        foreach(TARGET_FOLDER IN LISTS TARGET_FOLDERS)
            set(PCH_FILE "${LIBERTY_SOURCE_DIR}/${TARGET_FOLDER}/${TARGET_FOLDER}-PCH.h")
            if(EXISTS "${PCH_FILE}")
                target_precompile_headers(${TARGET_NAME} PUBLIC ${PCH_FILE})
            endif()
            target_include_directories(${TARGET_NAME} PUBLIC "${LIBERTY_SOURCE_DIR}/${TARGET_FOLDER}")
        endforeach()
        target_include_directories(${TARGET_NAME} PRIVATE "${DXSDK_INCLUDE_DIR}")
        target_include_directories(${TARGET_NAME} PRIVATE "${LIBERTY_SOURCE_DIR}")
        target_include_directories(${TARGET_NAME} PRIVATE "${LIBERTY_GENERATED_DIR}")
        target_compile_definitions(${TARGET_NAME} PRIVATE "${TARGET_NAME_UPPER}_PRIVATE")
        target_compile_definitions(${TARGET_NAME} PRIVATE "_CRT_SECURE_NO_WARNINGS")
        target_compile_definitions(${TARGET_NAME} PRIVATE "$<$<CONFIG:Debug>:_DEBUG>")
        target_compile_definitions(${TARGET_NAME} PUBLIC "_ALLOW_KEYWORD_MACROS")
    else()
        set_property(TARGET ${TARGET_NAME} PROPERTY VS_GLOBAL_IncludePath "${DXSDK_INCLUDE_DIR};${LIBERTY_SOURCE_DIR};${LIBERTY_GENERATED_DIR};$(LLVMInstallDir)/lib/clang/$(LLVMIncludeVersion)/include")
    endif()
    
    create_source_groups("${LIBERTY_SOURCE_DIR}" "${SOURCES}")
    create_source_groups("${LIBERTY_SOURCE_DIR}" "${CONFIG_SOURCES}")
    create_source_groups("${LIBERTY_GENERATED_DIR}" "${GENERATED_SOURCES}")
endfunction()

function(add_liberty_cs_project TARGET_NAME TARGET_TYPE TARGET_FOLDERS TARGET_SUBDIRECTORY)
    string(TOUPPER "${TARGET_NAME}" TARGET_NAME_UPPER)

    set(SOURCES "")
    if(TARGET_FOLDERS)
        foreach(TARGET_FOLDER IN LISTS TARGET_FOLDERS)
            file(GLOB_RECURSE TARGET_FILES
                "${LIBERTY_SOURCE_DIR}/${TARGET_FOLDER}/*.cs")
            list(APPEND SOURCES ${TARGET_FILES})
        endforeach()
    else()
        file(GLOB_RECURSE TARGET_FILES
            "${LIBERTY_SOURCE_DIR}/*.cs")
        list(APPEND SOURCES ${TARGET_FILES})
    endif()

    if(TARGET_TYPE STREQUAL "executable")
        add_executable(${TARGET_NAME} ${SOURCES})
    elseif(TARGET_TYPE STREQUAL "generator")
        add_executable(${TARGET_NAME} ${SOURCES})
    
        set(GENERATED_SOURCES "")
        set(GENERATED_DEPENDS "")
        generate_generated_sources("${TARGET_NAME}" "${TARGET_TYPE}" "${TARGET_FOLDERS}")

        # Define a custom command that runs the generator executable and produces the generated sources.
        add_custom_command(
            OUTPUT ${GENERATED_SOURCES}
            COMMAND "$<TARGET_FILE:${TARGET_NAME}>" "export_liberty"
            COMMAND ${CMAKE_COMMAND} -E echo "Generated Sources: ${GENERATED_SOURCES}"
            DEPENDS ${TARGET_NAME} ${GENERATED_DEPENDS}
            COMMENT "Running C# Generator")

    endif()
        
    set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${LIBERTY_DIRECTORY}/${TARGET_SUBDIRECTORY}")
    set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${LIBERTY_DIRECTORY}/${TARGET_SUBDIRECTORY}")
    set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${LIBERTY_DIRECTORY}/${TARGET_SUBDIRECTORY}")
    set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${LIBERTY_DIRECTORY}/${TARGET_SUBDIRECTORY}")
    set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${LIBERTY_DIRECTORY}/${TARGET_SUBDIRECTORY}")
    
    target_compile_definitions(${TARGET_NAME} PUBLIC "${TARGET_NAME_UPPER}")
    set_target_properties(${TARGET_NAME} PROPERTIES VS_GLOBAL_NoWarn "0169") # A private variable was declared but never referenced
    target_compile_options(${TARGET_NAME} PRIVATE "/langversion:9")
    
    create_source_groups("${LIBERTY_SOURCE_DIR}" "${SOURCES}")
endfunction()

add_liberty_cs_project(GlobalServer "executable" "GlobalServer" ${LIBERTY_TOOLS_DIRECTORY})
add_liberty_cs_project(CrashHandler "executable" "CrashHandler" ${LIBERTY_TOOLS_DIRECTORY})
set_target_properties(GlobalServer PROPERTIES FOLDER "Service")
set_target_properties(CrashHandler PROPERTIES FOLDER "Service")

add_liberty_project(Core "library" "Core" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(RemoteServer "shared" "RemoteServer" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(RemoteClient "shared" "RemoteClient" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(ReadFile "shared" "ReadFile" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(LuaProfile "shared" "LuaProfile" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(ImeUi "shared" "ImeUi" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(FLMaterials "shared" "FLMaterials" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(EngBase "shared" "EngBase" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(Deformable2 "shared" "Deformable2" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(DebugLib "shared" "DebugLib" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(DALib "shared" "DALib" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(DACOM "shared" "DACOM" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(Content "shared" "Content" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(Common "shared" "Common" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(Alchemy "shared" "Alchemy" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(zlib "shared" "zlib" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(ximage "shared" "ximage" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(x86math "shared" "x86math" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(THORN "shared" "THORN" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(System "shared" "System" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(SoundStreamer "shared" "SoundStreamer" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(SoundManager "shared" "SoundManager" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(Shading "shared" "Shading" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(Server "shared" "Server" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(RPCLocal "shared" "RPCLocal" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(RP8 "shared" "RP8" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(RendComp "shared" "RendComp" ${LIBERTY_EXE_DIRECTORY})

set_target_properties(Core PROPERTIES FOLDER "Library")
set_target_properties(RemoteServer PROPERTIES FOLDER "Library")
set_target_properties(RemoteClient PROPERTIES FOLDER "Library")
set_target_properties(ReadFile PROPERTIES FOLDER "Library")
set_target_properties(LuaProfile PROPERTIES FOLDER "Library")
set_target_properties(ImeUi PROPERTIES FOLDER "Library")
set_target_properties(FLMaterials PROPERTIES FOLDER "Library")
set_target_properties(EngBase PROPERTIES FOLDER "Library")
set_target_properties(Deformable2 PROPERTIES FOLDER "Library")
set_target_properties(DebugLib PROPERTIES FOLDER "Library")
set_target_properties(DALib PROPERTIES FOLDER "Library")
set_target_properties(DACOM PROPERTIES FOLDER "Library")
set_target_properties(Content PROPERTIES FOLDER "Library")
set_target_properties(Common PROPERTIES FOLDER "Library")
set_target_properties(Alchemy PROPERTIES FOLDER "Library")
set_target_properties(zlib PROPERTIES FOLDER "Library")
set_target_properties(ximage PROPERTIES FOLDER "Library")
set_target_properties(x86math PROPERTIES FOLDER "Library")
set_target_properties(THORN PROPERTIES FOLDER "Library")
set_target_properties(System PROPERTIES FOLDER "Library")
set_target_properties(SoundStreamer PROPERTIES FOLDER "Library")
set_target_properties(SoundManager PROPERTIES FOLDER "Library")
set_target_properties(Shading PROPERTIES FOLDER "Library")
set_target_properties(Server PROPERTIES FOLDER "Library")
set_target_properties(RPCLocal PROPERTIES FOLDER "Library")
set_target_properties(RP8 PROPERTIES FOLDER "Library")
set_target_properties(RendComp PROPERTIES FOLDER "Library")

add_liberty_project(FreeLancer "console" "FreeLancer" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(FLServer "console" "FLServer" ${LIBERTY_EXE_DIRECTORY})
set_target_properties(FreeLancer PROPERTIES FOLDER "Executable")
set_target_properties(FLServer PROPERTIES FOLDER "Executable")
set_target_properties(FreeLancer PROPERTIES OUTPUT_NAME "Liberty")
set_target_properties(FLServer PROPERTIES OUTPUT_NAME "LibertyServer")

add_liberty_project(Movie "shared" "Movie" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(EBUEula "shared" "EBUEula" ${LIBERTY_EXE_DIRECTORY})
set_target_properties(Movie PROPERTIES FOLDER "Stub")
set_target_properties(EBUEula PROPERTIES FOLDER "Stub")

add_liberty_project(Resources "resource" "Text/Resources" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(EquipResources "resource" "Text/EquipResources" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(NameResources "resource" "Text/NameResources" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(OfferBribeResources "resource" "Text/OfferBribeResources" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(ServerResources "resource" "Text/ServerResources" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(InfoCards "resource" "Text/InfoCards" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(MiscText "resource" "Text/MiscText" ${LIBERTY_EXE_DIRECTORY})
add_liberty_project(MiscTextInfo2 "resource" "Text/MiscTextInfo2" ${LIBERTY_EXE_DIRECTORY})
set_target_properties(Resources PROPERTIES FOLDER "Text")
set_target_properties(EquipResources PROPERTIES FOLDER "Text")
set_target_properties(NameResources PROPERTIES FOLDER "Text")
set_target_properties(OfferBribeResources PROPERTIES FOLDER "Text")
set_target_properties(ServerResources PROPERTIES FOLDER "Text")
set_target_properties(InfoCards PROPERTIES FOLDER "Text")
set_target_properties(MiscText PROPERTIES FOLDER "Text")
set_target_properties(MiscTextInfo2 PROPERTIES FOLDER "Text")

target_compile_definitions(ReadFile PUBLIC "READFILE_EXTRA_LOGGING")
target_compile_definitions(ReadFile PUBLIC "UTF_FILESYSTEM")
#target_compile_definitions(ReadFile PUBLIC "MEM_FILE_FILESYSTEM")
#target_compile_definitions(ReadFile PUBLIC "SEARCH_PATH_FILESYSTEM")
#target_compile_definitions(SoundManager PUBLIC "DA_THREAD_SAFE")

target_link_libraries(RemoteServer Core)
target_link_libraries(RemoteClient Core)
target_link_libraries(ReadFile Core DACOM Shlwapi.lib)
target_link_libraries(LuaProfile Core)
target_link_libraries(ImeUi Core)
target_link_libraries(FreeLancer Core)
target_link_libraries(FLServer Core)
target_link_libraries(FLMaterials Core)
target_link_libraries(EngBase Core)
target_link_libraries(Deformable2 Core)
target_link_libraries(DebugLib Core)
target_link_libraries(DALib Core)
target_link_libraries(DACOM Core Version.lib)
target_link_libraries(Content Core)
target_link_libraries(Common Core DACOM x86math)
target_link_libraries(Alchemy Core)
target_link_libraries(zlib Core)
target_link_libraries(ximage Core)
target_link_libraries(x86math Core DACOM)
target_link_libraries(THORN Core DACOM)
target_link_libraries(System Core)
target_link_libraries(SoundStreamer Core DACOM MSAcm32.Lib)
target_link_libraries(SoundManager Core DACOM x86math MSAcm32.Lib Winmm.lib)
target_link_libraries(Shading Core)
target_link_libraries(Server Core DACOM x86math)
target_link_libraries(RPCLocal Core)
target_link_libraries(RP8 Core DACOM x86math dxguid.lib)
target_link_libraries(RendComp Core)

# TODO target_include_libraries
# Need to add the include directories for these projects, but not
# hard link them because they use DACOM to query the interface
target_link_libraries(SoundManager ReadFile)
target_link_libraries(SoundStreamer ReadFile)
target_link_libraries(RP8 ReadFile)

add_dependencies(FreeLancer 
    RemoteServer
    RemoteClient
    ReadFile
    Movie
    LuaProfile
    ImeUi
    FLMaterials
    EngBase
    Deformable2
    DebugLib
    DALib
    DACOM
    Content
    Common
    Alchemy
    zlib
    ximage
    x86math
    THORN
    System
    SoundStreamer
    SoundManager
    Shading
    Server
    RPCLocal
    RP8
    RendComp
    Resources
    EquipResources
    NameResources
    OfferBribeResources
    ServerResources
    InfoCards
    MiscText
    MiscTextInfo2
    EBUEula)
add_dependencies(FLServer 
    RemoteServer
    RemoteClient
    ReadFile
    Movie
    LuaProfile
    ImeUi
    FLMaterials
    EngBase
    Deformable2
    DebugLib
    DALib
    DACOM
    Content
    Common
    Alchemy
    zlib
    ximage
    x86math
    THORN
    System
    SoundStreamer
    SoundManager
    Shading
    Server
    RPCLocal
    RP8
    RendComp
    Resources
    EquipResources
    NameResources
    OfferBribeResources
    ServerResources
    InfoCards
    MiscText
    MiscTextInfo2
    EBUEula)
    
set_target_properties(FreeLancer PROPERTIES VS_DEBUGGER_NATIVE_DEBUGGING TRUE)
set_property(TARGET FreeLancer PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${LIBERTY_DIRECTORY}/${LIBERTY_EXE_DIRECTORY}")
set_property(TARGET FreeLancer PROPERTY VS_DEBUGGER_ENVIRONMENT "DXVK_HUD=full")
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT FreeLancer)

set_target_properties(FLServer PROPERTIES VS_DEBUGGER_NATIVE_DEBUGGING TRUE)
set_property(TARGET FLServer PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${LIBERTY_DIRECTORY}/${LIBERTY_EXE_DIRECTORY}")

# Define the output file path in the binary directory (same as the .sln file)
set(SLN_LAUNCH_FILE "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.slnLaunch")
set(SLN_LAUNCH_FILE_CONTENT "[")
# Write the JSON content to the file
file(WRITE ${SLN_LAUNCH_FILE}
  "["
  "  {"
  "    \"Name\": \"FreeLancer\","
  "    \"Projects\": ["
  "      { \"Path\": \"FreeLancer.vcxproj\", \"Action\": \"Start\" },"
  "    ]"
  "  },"
  "  {"
  "    \"Name\": \"FLServer\","
  "    \"Projects\": ["
  "      { \"Path\": \"FLServer.vcxproj\", \"Action\": \"Start\" },"
  "      { \"Path\": \"GlobalServer.csproj\", \"Action\": \"Start\" }"
  "    ]"
  "  },"
  "  {"
  "    \"Name\": \"FreeLancer + FLServer\","
  "    \"Projects\": ["
  "      { \"Path\": \"FreeLancer.vcxproj\", \"Action\": \"Start\" },"
  "      { \"Path\": \"FLServer.vcxproj\", \"Action\": \"Start\" },"
  "      { \"Path\": \"GlobalServer.csproj\", \"Action\": \"Start\" }"
  "    ]"
  "  },"
  "]"
)
