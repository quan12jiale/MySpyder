##配置CMake版本
cmake_minimum_required(VERSION 3.13)

##配置项目名称/语言/版本
project(myspyder
    LANGUAGES CXX C
    VERSION 1.0
)

set(ISWINDOWS OFF)
set(ISLINUX OFF)

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    SET(ISWINDOWS ON)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    SET(ISLINUX ON)
else()
	MESSAGE(FATAL_ERROR "Has not support your system:${CMAKE_SYSTEM_NAME}...")
endif()

##Linux区分架构
if(${ISLINUX})
    if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64")
        add_definitions("-D__X86_64__")
    elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "aarch64")
        add_definitions("-D__AARCH64__")
    elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "mips64")
        add_definitions("-D__MIPS64__")
    else()
        message(STATUS "host processor architecture is unknown")
    endif()
endif()

##配置构建类型
if(${ISLINUX})
    set(CMAKE_CONFIGURATION_TYPES Debug Release ReleaseDebug)
endif()

set(QT_PREFIX_PATH $ENV{QTDIR}) # QTDIR64
message("QT install path: $ENV{QTDIR}") # QTDIR64

if(${ISWINDOWS})
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		set(PROJECT_PLATFORM "Win32")
		set(PROJECT_PLATFORM_X "X86")
		add_definitions(-DWIN32)
	elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(PROJECT_PLATFORM "x64")
		set(PROJECT_PLATFORM_X "X64")
		add_definitions(-DWIN64)
	endif()
else()
    set(PROJECT_PLATFORM "x64")
    set(PROJECT_PLATFORM_X "X64")
endif()

list(APPEND CMAKE_PREFIX_PATH ${QT_PREFIX_PATH})

##源代码位置
set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../")

##全局配置(并行编译、设置内联函数展开、关闭警告、启用comdat折叠,Release版本生成PDB)
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /Ob2 /W0")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}") # /OPT:REF /OPT:ICF
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-stack-protector")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-stack-protector")
    add_compile_options(-fpermissive)
	add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
    add_compile_options(-Wno-narrowing)
endif()

##禁止RelWithDebInfo的优化，打开警告
option(BUILD_DEVELOPER "Build for developer" ON)
if(MSVC AND BUILD_DEVELOPER)
    STRING(REPLACE "/O2" "/Od" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}) 
	STRING(REPLACE "/W0" "/W3" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
endif()

if(${ISLINUX})
    ##Release版本内联函数扩展改为默认值
    STRING(REPLACE "/Ob1" "" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})

    ##配置ReleaseDebug版本
    set(CMAKE_C_FLAGS_RELEASEDEBUG ${CMAKE_C_FLAGS_RELWITHDEBINFO})
    set(CMAKE_CXX_FLAGS_RELEASEDEBUG ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
    SET(CMAKE_EXE_LINKER_FLAGS_RELEASEDEBUG ${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO})
    SET(CMAKE_SHARED_LINKER_FLAGS_RELEASEDEBUG ${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO})

    STRING(REPLACE "/O2" "/Od" CMAKE_CXX_FLAGS_RELEASEDEBUG ${CMAKE_CXX_FLAGS_RELEASEDEBUG})
    STRING(REPLACE "-O2" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
    STRING(REPLACE "/Ob1" "" CMAKE_CXX_FLAGS_RELEASEDEBUG ${CMAKE_CXX_FLAGS_RELEASEDEBUG})

    #降低优化级别
    STRING(REPLACE "-O3" "-O1" CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
    STRING(REPLACE "-O3" "-O1" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})

    ##设置Debug版本的文件结尾符
    set(CMAKE_DEBUG_POSTFIX d)

    set(CMAKE_SKIP_RPATH OFF)
    set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
    set(CMAKE_BUILD_RPATH "\$ORIGIN")
endif()

##添加 lib路径
link_directories("${PROJECT_SOURCE_DIR}/../build/lib/${PROJECT_PLATFORM}/$<CONFIG>") # $(Configuration)

##控制VS工程生成使用"文件夹"组织结构
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

##将动态库\静态库输出路径定位到特定路径,供调试时使用(否则依赖的库分布在各个文件夹)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}../build/bin/${PROJECT_PLATFORM}/$<CONFIG>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}../build/lib/${PROJECT_PLATFORM}/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}../build/lib/${PROJECT_PLATFORM}/$<CONFIG>")
if(${ISLINUX})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/../build/bin/${PROJECT_PLATFORM}/$<CONFIG>")
endif()


# 依赖的包，要提前找
find_package(Qt5Core REQUIRED )
find_package(Qt5Gui REQUIRED )
find_package(Qt5Widgets REQUIRED )


add_subdirectory(spyder)

# 设置启动项
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT spyder)