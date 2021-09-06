cmake_minimum_required (VERSION 3.12 FATAL_ERROR)
project ("proxy" CXX C)

set_property (GLOBAL PROPERTY USE_FOLDERS ON)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_EXTENSIONS OFF)

if (WIN32) 
	add_definitions(-D_WIN32_WINNT=0x0601)
endif ()

if (MSVC)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
	
	option (ENABLE_STATIC_LINK "Build project with /MT flag" ON)
	mark_as_advanced(ENABLE_STATIC_LINK)
	if (ENABLE_STATIC_LINK STREQUAL ON)
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
		set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MT")
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
		set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MT")
	endif ()
	
else ()
	option (ENABLE_STATIC_LINK "Link libraries static")
	mark_as_advanced(ENABLE_STATIC_LINK)
endif ()



if ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
	if ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "x86_64")
		set(ARCH_HINT "x86_64")
		set(OUT_LIB_SUFFIX "")
		set(CMAKE_LIBRARY_ARCHITECTURE "x86_64-linux-gnu")
	else ()
		set(ARCH_HINT "i386")
		set(OUT_LIB_SUFFIX "32")
		set(CMAKE_LIBRARY_ARCHITECTURE "i386-linux-gnu")
	endif ()
elseif (MSVC AND ("${CMAKE_SYSTEM_NAME}" MATCHES "Windows"))
	if ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "x86_64")
		set(ARCH_HINT "x86_64")
		set(OUT_LIB_SUFFIX "")
		set(CMAKE_LIBRARY_ARCHITECTURE "x86_64-windows-msvc")
	elseif ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "AMD64")
		set(ARCH_HINT "AMD64")
		set(OUT_LIB_SUFFIX "")
		set(CMAKE_LIBRARY_ARCHITECTURE "amd64-windows-msvc")
	else ()
		set(ARCH_HINT "i386")
		set(OUT_LIB_SUFFIX "32")
		set(CMAKE_LIBRARY_ARCHITECTURE "i386-windows-msvc")
	endif ()
else ()
	message (FATAL_ERROR "${CMAKE_SYSTEM_NAME} not supported")
endif()


set (INCLUDES "")
set (LIBS "")

if (NOT WIN32)
	set(PROJECT_BUILD_TYPE "Release" CACHE STRING "Choose build type")
		set_property(CACHE PROJECT_BUILD_TYPE PROPERTY STRINGS "Release" "Debug" "RelWithDebInfo")
		set(CMAKE_BUILD_TYPE ${PROJECT_BUILD_TYPE})

		if(NOT (CMAKE_BUILD_TYPE STREQUAL Release OR CMAKE_BUILD_TYPE STREQUAL Debug OR CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo))
			message(WARNING "available only this build types: [Release],[Debug], [RelWithDebInfo]")
			set(CMAKE_BUILD_TYPE Release)
			message(STATUS "default build type - ${CMAKE_BUILD_TYPE}")
		else(NOT (CMAKE_BUILD_TYPE STREQUAL Release OR CMAKE_BUILD_TYPE STREQUAL Debug OR CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo))
			message(STATUS "build type - ${CMAKE_BUILD_TYPE}")
		endif(NOT (CMAKE_BUILD_TYPE STREQUAL Release OR CMAKE_BUILD_TYPE STREQUAL Debug OR CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo))
		message(STATUS "for changing build type choose:  -DPROJECT_BUILD_TYPE={Release|Debug|RelWithDebInfo}")
endif ()


set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set (Boost_USE_MULTITHREADED ON)
set (Boost_FIND_QUIETLY ON)
if ("${ENABLE_STATIC_LINK}" STREQUAL "ON")
	set (Boost_USE_STATIC_LIBS ON)
	set (Boost_USE_STATIC_RUNTIME ON)
endif()

find_package (Boost 1.67 REQUIRED COMPONENTS system filesystem thread regex timer)
if (Boost_FOUND)
	if (MSVC)
		# suppress messages about unknown compiler for MSVC, because
		# compiler updates occure faster than boost releases :-)
		add_definitions (-DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE)
	endif ()
	
	set (INCLUDES ${INCLUDES} ${Boost_INCLUDE_DIRS})
	set (LIBS ${LIBS} Boost::boost Boost::filesystem Boost::thread Boost::regex Boost::timer)
endif ()


include_directories (${INCLUDES})
include_directories (${CMAKE_CURRENT_SOURCE_DIR})

set (SRC_FILES 
	src/Proxy.h
	src/main.cpp
	src/Server.h
	src/Server.cpp
	src/Client.h
	src/Client.cpp
	src/Remote.h
	src/Remote.cpp
)


add_executable (proxy ${SRC_FILES})
target_link_libraries (proxy ${LIBS})