##配置CMake版本
cmake_minimum_required(VERSION 3.13)

##配置项目名称/语言/版本
project(MySpyder
    LANGUAGES CXX C
    VERSION 1.0
)


##源代码位置
set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../")


set(QT_PREFIX_PATH $ENV{QTDIR})
list(APPEND CMAKE_PREFIX_PATH ${QT_PREFIX_PATH})
message("QT install path: $ENV{QTDIR}")

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(PROJECT_PLATFORM "Win32")
    set(PROJECT_PLATFORM_X "X86")
    add_definitions(-DWIN32)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PROJECT_PLATFORM "x64")
    set(PROJECT_PLATFORM_X "X64")
    add_definitions(-DWIN64)
endif()


##全局配置(并行编译、设置内联函数展开、关闭警告、启用comdat折叠,Release版本生成PDB)
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /Ob2 /W0")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /OPT:REF /OPT:ICF")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG")
endif()

##禁止RelWithDebInfo的优化，打开警告
option(BUILD_DEVELOPER "Build for developer" ON)
if(MSVC AND BUILD_DEVELOPER)
    STRING(REPLACE "/O2" "/Od" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}) 
	STRING(REPLACE "/W0" "/W3" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
endif()

##添加 lib路径
link_directories("${PROJECT_SOURCE_DIR}/../build/lib/${PROJECT_PLATFORM}/$(Configuration)")

##控制VS工程生成使用"文件夹"组织结构
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

##将动态库\静态库输出路径定位到特定路径,供调试时使用(否则依赖的库分布在各个文件夹)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}../build/bin/${PROJECT_PLATFORM}/$<CONFIG>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}../build/lib/${PROJECT_PLATFORM}/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}../build/lib/${PROJECT_PLATFORM}/$<CONFIG>")
#message(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})


# 依赖的包，要提前找
find_package(Qt5Core REQUIRED )
find_package(Qt5Gui REQUIRED )
find_package(Qt5Widgets REQUIRED )


add_subdirectory(spyder)
add_subdirectory(musicplayer)

# 设置启动项
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT spyder)