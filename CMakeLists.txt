cmake_minimum_required(VERSION 2.8.12)
project(facer)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

include(../dlib/cmake)

if (dlib_NO_GUI_SUPPORT)
   message("No GUI support, so we won't build the facer project.")
else()
   find_package(OpenCV QUIET)
   if (OpenCV_FOUND)
      include_directories(${OpenCV_INCLUDE_DIRS})
	  include_directories("${PROJECT_SOURCE_DIR}/../")
	  include_directories("${PROJECT_SOURCE_DIR}/../../sqlite3")
	  include_directories("${PROJECT_SOURCE_DIR}/../../ffmpeg-3.4")
	  include_directories("${PROJECT_SOURCE_DIR}/../../Program Files (x86)/IntelSWTools/compilers_and_libraries_2018.1.156/windows/mkl/include")
 
	  add_executable(facer facer.cpp)
	  target_link_libraries(facer dlib::dlib ${OpenCV_LIBS} "${PROJECT_SOURCE_DIR}/../../sqlite3/sqlite3.lib" "${PROJECT_SOURCE_DIR}/mkl_lapack95.lib" "${PROJECT_SOURCE_DIR}/libfacedetect.lib")
	  
   else()
      message("OpenCV not found, so we won't build the project.")
   endif()
endif()
