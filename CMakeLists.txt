#
# Copyright (c) 2019 Arnaud Loonstra.
#
# License MPLv2
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

cmake_minimum_required(VERSION 3.1)
project(gazebosc)

set(CMAKE_CXX_STANDARD 14)
set(DEP_IMGUI_VERSION "1.79" CACHE STRING "ImGui version to use")
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
##  Test availability of git and fetch submodules if needed
#find_package(Git QUIET)
#if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
#    # Update submodules as needed
#    option(GIT_SUBMODULE "Check submodules during build" ON)
#    if(GIT_SUBMODULE)
#        message(STATUS "Submodule update")
#        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
#            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#            RESULT_VARIABLE GIT_SUBMOD_RESULT)
#        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
#            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
#        endif()
#    endif()
#endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/ext/czmq/CMakeLists.txt")
    message(FATAL_ERROR "The submodules in the ext directory are not available! GIT_SUBMODULE was turned off or failed. Please update submodules using \"git submodule update --init --recursive\" and try again.")
endif()
## End getting the submodules

### SET OPTIONS
option (WITH_UNWIND "Enable libunwind support" ON)
option (WITH_EMBED_PYTHON "Embed the python interpreter, otherwise use system installed" OFF)
### END SET OPTIONS

### EXTERNAL LIBS
# Definitions for draft API's of submodules
set(ENABLE_DRAFTS ON CACHE BOOL "enable zmq projects draft apis" FORCE)
add_definitions(-DZMQ_BUILD_DRAFT_API=1)
add_definitions(-DCZMQ_BUILD_DRAFT_API=1)

# External libraries in ext/
add_subdirectory("ext/czmq" EXCLUDE_FROM_ALL)
# overrides to prevent the Findczmq.cmake script to fail
set(czmq_LIBRARIES czmq-static)
set(czmq_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/ext/czmq/include/)
add_subdirectory("ext/libsphactor" EXCLUDE_FROM_ALL)
add_subdirectory("ext/SDL-mirror" EXCLUDE_FROM_ALL)
add_subdirectory("ext/glm" EXCLUDE_FROM_ALL)

# External system libraries through find_package
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}")
find_package(libzmq REQUIRED)
if (NOT libzmq_FOUND)
    message (FATAL_ERROR "Cannot find libzmq, make sure it is available on the system")
endif()

# Find Python3
if(Python3_ROOT_DIR)                        # Usually specified on command line to find python explicitly
    set(Python3_FIND_STRATEGY "LOCATION")
endif(Python3_ROOT_DIR)
find_package (Python3 REQUIRED COMPONENTS Development)
if(Python3_FOUND)
    message ("python3 found")
    add_definitions(-DPYTHON3_FOUND)
else()
    message (FATAL_ERROR "Cannot find Python3, make sure the development packages of Python are available on the system")
endif()

if (WITH_UNWIND)
    find_package (Unwind)
endif (WITH_UNWIND)
if (Unwind_FOUND)
    add_definitions(-DHAVE_LIBUNWIND)
    set (HAVE_LIBUNWIND 1)
    set (HAVE_UNWIND_H 1)
endif (Unwind_FOUND)

# include Dear ImGui
if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}.zip")
        file(DOWNLOAD "https://github.com/ocornut/imgui/archive/v${DEP_IMGUI_VERSION}.zip" ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}.zip)
    endif ()
    if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}")
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}.zip)
    endif ()
endif ()

### PROJECT SOURCES
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Actors ACTOR_SOURCES)
file (GLOB ACTOR_HEADERS Actors/*.h)

if (UNIX)
    link_directories(
        /usr/local/lib/
    )
endif()

add_executable(gazebosc
    main.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/examples/imgui_impl_opengl3.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/examples/imgui_impl_opengl3.h
    ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/examples/imgui_impl_sdl.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/examples/imgui_impl_sdl.h
    ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/imconfig.h
    ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/imgui.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/imgui.h
    ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/imgui_draw.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/imgui_widgets.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/imgui_internal.h
    ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/imstb_rectpack.h
    ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/imstb_textedit.h
    ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/imstb_truetype.h
    ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/examples/libs/gl3w/GL/gl3w.c
    ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/examples/libs/gl3w/GL/gl3w.h
    ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/examples/libs/gl3w/GL/glcorearb.h
    ImNodes.h
    ImNodes.cpp
    ImNodesEz.h
    ImNodesEz.cpp
    ext/ImGui-Addons/FileBrowser/ImGuiFileBrowser.h
    ext/ImGui-Addons/FileBrowser/ImGuiFileBrowser.cpp
    fontawesome5.h
    stage.cpp
    ActorContainer.h
    actors.h
    ${ACTOR_SOURCES}
    ${ACTOR_HEADERS}
)

target_include_directories(gazebosc PUBLIC
    ${Unwind_INCLUDE_DIRS}
    ${libzmq_INCLUDE_DIRS}
    ${Python3_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}
    ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/examples
    ${CMAKE_CURRENT_BINARY_DIR}/imgui-${DEP_IMGUI_VERSION}/examples/libs/gl3w
)

if (Unwind_FOUND)
  target_link_libraries (gazebosc PUBLIC unwind::unwind)
endif (Unwind_FOUND)

### PLATFORM SPECIFICS
if (APPLE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-stack-check")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-stack-check")
    add_definitions(-D__MACOSX_CORE__)

    target_link_libraries(gazebosc PUBLIC
        SDL2-static
        sphactor-static
        Python3::Python
        -ldl "-framework OpenGL" "-framework CoreFoundation" "-framework CoreMidi" "-framework CoreAudio")
    SET_TARGET_PROPERTIES(gazebosc PROPERTIES
        MACOSX_BUNDLE TRUE 
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/dist/osx/Info.plist.in
    )
    SET(MACOSX_BUNDLE_BUNDLE_NAME gazebosc)
    set(MACOSX_BUNDLE_COPYRIGHT "Gazebosc is licensed under the MPLv2 License")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.sphactor.Gazebosc")
    set(MACOSX_BUNDLE_ICON_FILE "AppIcon.icns")
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/dist/osx/AppIcon.iconset DESTINATION ${CMAKE_BINARY_DIR})
    set(ICON_TARGET "${CMAKE_BINARY_DIR}/AppIcon.iconset")
    set(ICON_OUTPUT "${CMAKE_BINARY_DIR}/AppIcon.icns")
    set(SOURCE_ICON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/misc/logo")
    add_custom_command(OUTPUT ${ICON_OUTPUT}
        COMMAND cp icon_x16.png   ${ICON_TARGET}/icon_16x16.png
        COMMAND cp icon_x32.png   ${ICON_TARGET}/icon_16x16@2x.png
        COMMAND cp icon_x32.png   ${ICON_TARGET}/icon_32x32.png
        COMMAND cp icon_x64.png   ${ICON_TARGET}/icon_32x32@2x.png
        COMMAND cp icon_x128.png  ${ICON_TARGET}/icon_128x128.png
        COMMAND cp icon_x256.png  ${ICON_TARGET}/icon_128x128@2x.png
        COMMAND cp icon_x256.png  ${ICON_TARGET}/icon_256x256.png
        COMMAND cp icon_x512.png  ${ICON_TARGET}/icon_256x256@2x.png
        COMMAND cp icon_x512.png  ${ICON_TARGET}/icon_512x512.png
        COMMAND cp icon_x1024.png ${ICON_TARGET}/icon_512x512@2x.png
        COMMAND iconutil -c icns ${ICON_TARGET}
        WORKING_DIRECTORY ${SOURCE_ICON_DIR}
    )
    set(BUNDLE_RESOURCES ${ICON_OUTPUT})
#    list(APPEND BUNDLE_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG.md)
#    list(APPEND BUNDLE_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/CONTRIBUTORS.MD)
    set_target_properties(${PROJECT} PROPERTIES RESOURCE "${BUNDLE_RESOURCES}")
    target_sources(gazebosc PUBLIC ${BUNDLE_RESOURCES})
#    file(GLOB BUNDLE_LANGUAGES "${CMAKE_CURRENT_SOURCE_DIR}/data/language/*")
#    target_sources(${PROJECT} PUBLIC ${BUNDLE_LANGUAGES})
#    set_property(
#        SOURCE ${BUNDLE_LANGUAGES}
#        PROPERTY MACOSX_PACKAGE_LOCATION "Resources/language"
#    )
elseif (WIN32)
    target_link_libraries(gazebosc PUBLIC
        SDL2-static
        czmq-static
        sphactor-static
        ${libzmq_LIBRARIES}
        Python3::Python
        opengl32.lib)
else()
    target_link_libraries(gazebosc PUBLIC
        SDL2-static
        sphactor-static
        Python3::Python
        -lasound -ldl -lGL -lGLEW)
    target_compile_options(gazebosc PUBLIC -D__LINUX_ALSA__ )
endif()
target_compile_options(gazebosc PUBLIC ${SDL2_CFLAGS_OTHER} )
target_compile_definitions(gazebosc PUBLIC -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS=1)

### POST BUILD/INSTALL
if(APPLE)
    install(TARGETS gazebosc BUNDLE DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    set(APPS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.app")
    set(LIBS )
    set(DIRS "${CMAKE_BINARY_DIR}")
    if (WITH_EMBED_PYTHON )
        # copy embedded python to the bundle but exclude exe_with_* files as it kills fixup_bundle
        install(DIRECTORY ${CMAKE_BINARY_DIR}/python DESTINATION ${APPS}/Contents/Resources
            USE_SOURCE_PERMISSIONS
            PATTERN exe_with_* EXCLUDE
        )
    endif(WITH_EMBED_PYTHON)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/misc DESTINATION ${APPS}/Contents/Resources)
    install(FILES ${CMAKE_SOURCE_DIR}/tester.py DESTINATION ${APPS}/Contents/Resources)
    # Path used for searching by FIND_XXX(), with appropriate suffixes added
    if(CMAKE_PREFIX_PATH)
        foreach(dir ${CMAKE_PREFIX_PATH})
                list(APPEND DIRS "${dir}/bin" "${dir}/lib")
        endforeach()
    endif()
    if (WITH_EMBED_PYTHON )
        # only make the bundle standalone if we are embedding python
        install(CODE "
            include(BundleUtilities)
            fixup_bundle(\"${APPS}\" \"${LIBS}\" \"${DIRS}\" IGNORE_ITEM \"exe_with_z64;exe_with_zip\")"
           )
    endif(WITH_EMBED_PYTHON)
else()
    install(TARGETS gazebosc DESTINATION bin)
    add_custom_command(
        TARGET gazebosc POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/misc
                $<TARGET_FILE_DIR:${PROJECT_NAME}>/misc)

endif(APPLE)

add_custom_command(
    TARGET gazebosc POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/tester.py
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/tester.py)

