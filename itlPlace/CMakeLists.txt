cmake_minimum_required(VERSION 2.8)
project(itlPlace)

# This is for debugging on Win only
#SET(OpenCV_DIR C:/Kornyakov/opencv-build-qtcreator)
#SET(PETSC_DIR c:/Kornyakov/itlplace-on-mac/itlPlace/3rdparty/PETSc)
#SET(PETSC_ARCH X86_64)
#SET(MAKE_EXECUTABLE nmake)

MESSAGE("OpenCV_DIR: ${OpenCV_DIR}")
MESSAGE("PETSC_DIR:  ${PETSC_DIR}")
MESSAGE("PETSC_ARCH: ${PETSC_ARCH}")
MESSAGE("TAO_DIR:    ${TAO_DIR}")

#TODO: hardcoded paths
set(FOR_GOOGLE_HASH "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk/usr/include/c++/4.2.1/")
set(TAO_LIBRARIES_PATH "${TAO_DIR}/lib/${PETSC_ARCH}")

# Optional commands
#SET(CMAKE_BUILD_TYPE Release)
#SET(CMAKE_CXX_FLAGS "-ferror-limit=3")

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

FIND_PACKAGE(PETSc COMPONENTS CXX)
SET(PETSC_INCLUDES "${PETSC_INCLUDES};${PETSC_DIR}/include/mpiuni")
SET(PETSC_LIBRARIES "${PETSC_LIBRARIES};${PETSC_DIR}/lib/${PETSC_ARCH}/libf2clapack.a;${PETSC_DIR}/lib/${PETSC_ARCH}/libf2cblas.a;${PETSC_DIR}/lib/${PETSC_ARCH}/libmpiuni.a")

FIND_PACKAGE(OpenCV REQUIRED)
MESSAGE("OpenCV_LIBS: ${OpenCV_LIBS}")

# Glob for headers
MACRO(HEADER_DIRECTORIES return_list)
    FILE(GLOB_RECURSE new_list "include/*.h" "src/*.h")
    SET(dir_list "include")
    FOREACH(file_path ${new_list})
        GET_FILENAME_COMPONENT(dir_path ${file_path} PATH)
        SET(dir_list ${dir_list} ${dir_path})
    ENDFOREACH()
    LIST(REMOVE_DUPLICATES dir_list)
    SET(${return_list} ${dir_list})
ENDMACRO()

HEADER_DIRECTORIES(hdrs)
INCLUDE_DIRECTORIES(${hdrs} ${FOR_GOOGLE_HASH})
INCLUDE_DIRECTORIES(${hdrs} ${PETSC_INCLUDES})

SET(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -stdlib=libstdc++")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")

LINK_DIRECTORIES(${TAO_LIBRARIES_PATH})
SET(TAO_LIBRARIES tao taopetsc)

# Add application
SET(app "itlPlace")
FILE(GLOB_RECURSE srcs "include/*.h*" "src/*.cpp" "src/*.c" "src/*.h*" "src/config/*.c*") #"src/*.y"
ADD_EXECUTABLE(${app} ${srcs})
TARGET_LINK_LIBRARIES(${app} ${OpenCV_LIBS} ${TAO_LIBRARIES} ${PETSC_LIBRARIES})

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ln -fns "${CMAKE_SOURCE_DIR}/cfg" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/cfg"
    COMMAND ln -fns "${CMAKE_SOURCE_DIR}/technology" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/technology"
)

# Debugging CMake
FUNCTION(JOIN values separator result)
  STRING(REPLACE ";" "${separator}" _TMP_STR "${values}")
  SET(${result} "${_TMP_STR}" PARENT_SCOPE)
ENDFUNCTION()

JOIN("${OpenCV_LIBS}" "\n" output)
#MESSAGE("Hey: ${output}")
