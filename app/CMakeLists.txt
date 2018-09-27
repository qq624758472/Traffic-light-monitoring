# Sets the minimum version of CMake required to build the native
# library. You should either keep the default value or only pass a
# value of 3.4.0 or lower.

#CMake版本信息
cmake_minimum_required(VERSION 3.4.1)
#工程路径
set(pathToProject E:/Cmake )
#opencv4android sdk路径
set(pathToOpenCV  E:/Opencv4Android/OpenCV-android-sdk )
#配置加载native依赖
include_directories(${pathToOpenCv}/sdk/native/jni/include)#包含掉OpenCv的头文件目录
#配置cpp下include的路径
set(CMAKE_VERBOSE_MAKEFILE on)
set(libs "${CMAKE_SOURCE_DIR}/src/main/jniLibs")
include_directories(${CMAKE_SOURCE_DIR}/src/main/cpp/include)
#自己待编译的cpp源文件
add_library( native-lib2 SHARED src/main/cpp/native-lib.cpp)
#动态加载方式
add_library(lib_opencv STATIC IMPORTED ) #表示创建一个导入库，静态方式
#引入libopencv_java.so文件
set_target_properties(lib_opencv PROPERTIES IMPORTED_LOCATION ${pathToProject}/app/src/main/jniLibs/${ANDROID_ABI}/libopencv_java.so)#包含libopencv_java.so这个库
#c++日志
find_library( log-lib log )

target_link_libraries( native-lib2 $\{log-lib} lib_opencv)


