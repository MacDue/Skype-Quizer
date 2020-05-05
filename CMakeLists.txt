cmake_minimum_required (VERSION 3.6)
project(quiz)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "-g -fconcepts -Wno-init-list-lifetime")
set(CMAKE_CXX_FLAGS_RELEASE "-march=native -O3 -fconcepts -g -flto")

if (CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
endif()

add_compile_definitions(cimg_use_png)
add_definitions(-DDUEUTIL_HOST=1 -DUNIX_HOST=1 -Dcimg_display=0)

find_package(PNG REQUIRED)
link_libraries(${PNG_LIBRARY})

# FreeType
set(ENV{FREETYPE_DIR} /usr/local/lib/)
find_package(Freetype REQUIRED)

include_directories(
  ${PNG_INCLUDE_DIR}
  ${FREETYPE_INCLUDE_DIRS}
  ${PostgreSQL_INCLUDE_DIRS})

message(STATUS "${FREETYPE_VERSION_STRING}=${${FREETYPE_VERSION_STRING}}")
message(STATUS "${FREETYPE_LIBRARIES}=${${FREETYPE_LIBRARIES}}")
message(STATUS "${FREETYPE_INCLUDE_DIRS}=${${FREETYPE_INCLUDE_DIRS}}")

set(PROJECT_DEPS_DIR "${PROJECT_SOURCE_DIR}/deps")

include_directories(
  ./src
  ./CImg-2.8.1
  ./crow/include
  ./litexx/include
  ./formatxx/include
  ./rapidjson/include)

file(GLOB_RECURSE source_list CONFIGURE_DEPENDS "./src/*.cpp")

add_executable(quiz ${source_list} ./formatxx/source/format.cc)

execute_process (
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMAND ln -s ../assets build/assets)

target_link_libraries(quiz stdc++fs boost_system pthread ${FREETYPE_LIBRARIES} ${PostgreSQL_LIBRARIES})
