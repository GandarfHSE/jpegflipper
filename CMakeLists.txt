cmake_minimum_required(VERSION 3.2)

project(jpegflipper)

# Находим путь до папки с заголовочными файлами и записываем в ${LIBEVENT_INCLUDE_DIR}
find_path(LIBEVENT_INCLUDE_DIR event.h
  PATHS
    /usr/local
    /opt
  PATH_SUFFIXES
    include
)

# Находим бинарные файлы библиотеки и записываем в ${LIBEVENT_LIB}
find_library(LIBEVENT_LIB
  NAMES
    event
  PATHS
    /usr/local
    /opt
  PATH_SUFFIXES
    lib
    lib64
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  LIBEVENT_LIB
  LIBEVENT_INCLUDE_DIR
)

set(SRC_LIST main.cpp task.cpp threadpool.cpp)

add_executable(${PROJECT_NAME} ${SRC_LIST})

target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBEVENT_LIB})

target_include_directories(${PROJECT_NAME} PUBLIC ${LIBEVENT_INCLUDE_DIR})

# Находим Magick++
add_definitions( -DMAGICKCORE_QUANTUM_DEPTH=16 )
add_definitions( -DMAGICKCORE_HDRI_ENABLE=0 )

find_package(ImageMagick COMPONENTS Magick++)

include_directories(${ImageMagick_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PUBLIC ${ImageMagick_LIBRARIES})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
