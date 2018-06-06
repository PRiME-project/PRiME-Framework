if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "DO NOT BUILD in-tree. Please make and use build subdirectory.")
endif()

cmake_minimum_required(VERSION 2.8)
project (PRIME_Framework)

#==================================================================
# Check that framework dependencies are installed
# Boost
find_package(Boost REQUIRED COMPONENTS system thread)
if(NOT Boost_FOUND)
	message(FATAL_ERROR ">> Boost System and/or Boost Thread Not Installed")
endif()
# Pthreads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
# Rand
find_program(RAND_EXISTS rand)
if(NOT RAND_EXISTS)
	message(">> Rand Not Installed ")
endif()
#==================================================================

set(CMAKE_CXX_FLAGS "-std=gnu++11 ${CMAKE_CXX_FLAGS}")
SET(DEVICE "XU3" CACHE STRING "The type of device to compile. Default is Odroid XU3")

include_directories(lib/include)

add_library(procutils lib/procutils.c lib/at.c) 
set_target_properties(procutils PROPERTIES COMPILE_FLAGS "-DHAVE_NANOSLEEP")

add_library(uds lib/uds.cpp)
target_link_libraries(uds LINK_PUBLIC boost_system boost_thread pthread)
add_library(util lib/util.cpp)
target_link_libraries(util LINK_PUBLIC uds  procutils)

add_library(prime_api_app lib/prime_api_app.cpp)
target_link_libraries(prime_api_app LINK_PUBLIC util)
add_library(prime_api_rtm lib/prime_api_rtm.cpp)
target_link_libraries(prime_api_rtm LINK_PUBLIC util)
add_library(prime_api_dev lib/prime_api_dev.cpp)
target_link_libraries(prime_api_dev LINK_PUBLIC util)
add_library(arch_utils rtm/rtm_arch_utils.cpp)
target_link_libraries(arch_utils LINK_PUBLIC util)

#==================================================================
#Components integrated into the framework.

#-------------------------- Applications --------------------------

#==================================================================
# Commmon Library Dependencies
#OpenCV
find_package(OpenCV REQUIRED COMPONENTS core calib3d imgproc highgui objdetect video)
if(OpenCV_FOUND)
	message(">> OpenCV-dependent Applications can be built.")
	include_directories(${OpenCV_INCLUDE_DIRS})
endif(OpenCV_FOUND)

#OpenCL
find_package(OpenCL REQUIRED)
if(OpenCL_FOUND)
	message(">> OpenCL-dependent Applications can be built.")
	include_directories(${OpenCL_INCLUDE_DIRS})
endif(OpenCL_FOUND)

#OpenMP
find_package(OpenMP)
include_directories(${OpenMP_INCLUDE_DIRS})
if(OPENMP_FOUND)
	message(">> OpenMP-dependent Applications can be built.")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif(OPENMP_FOUND)
#==================================================================

# Video_Decode
if(OpenCV_FOUND)
	message(">> Building Video Decode Application.")
	file(GLOB APP_DECODE_SOURCES app/video_decode/*.cpp)
	add_executable(app_decode ${APP_DECODE_SOURCES})
	target_link_libraries(app_decode LINK_PUBLIC uds boost_system boost_thread prime_api_app ${OpenCV_LIBS} pthread)
	if(RAND_EXISTS)
		set_target_properties(app_decode PROPERTIES COMPILE_FLAGS "-DCOMPILE_RAND=$$(rand) -DCOMPILE_EPOCH=$$(date +%s)")
	endif(RAND_EXISTS)
else(OpenCV_FOUND)
	message(">> OpenCV Required for Video Decode Application.")
endif(OpenCV_FOUND)

# Jacobi
file(GLOB APP_JACOBI_SOURCES app/jacobi/*.cpp)
add_executable(app_jacobi ${APP_JACOBI_SOURCES})
if(${DEVICE} STREQUAL "C5SOC")
	message(">> Building Jacobi for Altera OpenCL")
	execute_process(COMMAND aocl compile-config RESULT_VARIABLE AOCL_COMPILE_CONFIG_RESULT OUTPUT_VARIABLE AOCL_COMPILE_CONFIG OUTPUT_STRIP_TRAILING_WHITESPACE)
	if(NOT ${AOCL_COMPILE_CONFIG_RESULT} MATCHES 0)
		message(FATAL_ERROR ">> Call to aocl failed. Did you source init_opencl.sh?")
	endif()
	set_target_properties(app_jacobi PROPERTIES COMPILE_FLAGS "${AOCL_COMPILE_CONFIG} -DCOMPILE_RAND=$$(rand) -DCOMPILE_EPOCH=$$(date +%s) -DC5SOC")
	execute_process(COMMAND aocl ldflags OUTPUT_VARIABLE AOCL_LDFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
	execute_process(COMMAND aocl ldlibs OUTPUT_VARIABLE AOCL_LDLIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
	set_target_properties(app_jacobi PROPERTIES LINK_FLAGS "${AOCL_LDFLAGS} ${AOCL_LDLIBS}")
	target_link_libraries(app_jacobi LINK_PUBLIC uds boost_system boost_thread prime_api_app pthread kocl)
#	configure_file(app/jacobi/assets/jacobi.aocx jacobi.aocx COPYONLY)
#	configure_file(app/jacobi/assets/jacobi_kocl.aocx jacobi_kocl.aocx COPYONLY)
	configure_file(app/jacobi/assets/jacobi-256.aocx jacobi.aocx COPYONLY)
        configure_file(app/jacobi/assets/jacobi-256_kocl.aocx jacobi_kocl.aocx COPYONLY)
else()
	message(">> Building Jacobi for vanilla OpenCL")
	if(OpenCL_FOUND)
		target_link_libraries(app_jacobi LINK_PUBLIC uds boost_system boost_thread prime_api_app ${OpenCL_LIBRARIES} pthread ${OpenMP_LIBRARIES})
		if(RAND_EXISTS)	
			set_target_properties(app_jacobi PROPERTIES COMPILE_FLAGS "-DCOMPILE_RAND=$$(rand) -DCOMPILE_EPOCH=$$(date +%s)")
		endif(RAND_EXISTS)
	else(OpenCL_FOUND)
		message(">> OpenCL Required for Jacobi Application.")
	endif(OpenCL_FOUND)
endif()

# Whetstone
message(">> Building Whetstone Application.")
file(GLOB APP_WHETSTONE_SOURCES app/whetstone/*.cpp)
add_executable(app_whetstone ${APP_WHETSTONE_SOURCES})
target_link_libraries(app_whetstone LINK_PUBLIC uds boost_system boost_thread prime_api_app pthread)
if(RAND_EXISTS)
	set_target_properties(app_whetstone PROPERTIES COMPILE_FLAGS "-DCOMPILE_RAND=$$(rand) -DCOMPILE_EPOCH=$$(date +%s)")
endif(RAND_EXISTS)

#Stereo Vision
if(OpenCL_FOUND AND OpenCV_FOUND)
	message(">> Building Stereo Vision Application.")
	include_directories(app/stereo_vision/include)
	file(GLOB APP_STEREO_VISION_SOURCES app/stereo_vision/src/*.cpp)
	add_executable(app_stereo_vision ${APP_STEREO_VISION_SOURCES})
	target_link_libraries(app_stereo_vision LINK_PUBLIC uds boost_system boost_thread prime_api_app ${OpenCV_LIBS} ${OpenCL_LIBRARIES} pthread)
	if(RAND_EXISTS)
		set_target_properties(app_stereo_vision PROPERTIES COMPILE_FLAGS "-DCOMPILE_RAND=$$(rand) -DCOMPILE_EPOCH=$$(date +%s)")
	endif(RAND_EXISTS)
else()
	message(">> OpenCV and OpenCL Required for Stereo Vision Application.")
endif()

# Template App - for testing
message(">> Building Template Applications.")
add_executable(app_template app/app_template.cpp)
target_link_libraries(app_template LINK_PUBLIC uds boost_system boost_thread prime_api_app)
add_executable(app_template_with_ui app/app_template_with_ui.cpp)
target_link_libraries(app_template_with_ui LINK_PUBLIC uds boost_system boost_thread prime_api_app)
if(RAND_EXISTS)
	set_target_properties(app_template PROPERTIES COMPILE_FLAGS "-DCOMPILE_RAND=$$(rand) -DCOMPILE_EPOCH=$$(date +%s)")
	set_target_properties(app_template_with_ui PROPERTIES COMPILE_FLAGS "-DCOMPILE_RAND=$$(rand) -DCOMPILE_EPOCH=$$(date +%s)")
endif(RAND_EXISTS)

#------------------------------ RTMs ------------------------------
include_directories(rtm/include)

# Codegen Qlearn
message(">> Building Code-generated Q-learning RTM")
file(GLOB RTM_CODEGEN_QLEARN_SOURCES rtm/codegen_qlearn/*.cpp)
add_executable(rtm_codegen_qlearn ${RTM_CODEGEN_QLEARN_SOURCES})
target_link_libraries(rtm_codegen_qlearn LINK_PUBLIC uds boost_system boost_thread prime_api_rtm arch_utils)

# Codegen Qlearn + Mapping
file(GLOB RTM_CODEGEN_QLEARN_MAPPING_SOURCES rtm/codegen_qlearn_mapping/*.cpp)
add_executable(rtm_codegen_qlearn_mapping ${RTM_CODEGEN_QLEARN_MAPPING_SOURCES})
target_link_libraries(rtm_codegen_qlearn_mapping LINK_PUBLIC uds boost_system boost_thread prime_api_rtm arch_utils)

# Profiling
message(">> Building Profile RTM")
file(GLOB RTM_PROFILE_SOURCES rtm/profile/*.cpp)
add_executable(rtm_profile ${RTM_PROFILE_SOURCES})
target_link_libraries(rtm_profile LINK_PUBLIC uds boost_system boost_thread prime_api_rtm arch_utils)

# Arch Util Test
file(GLOB RTM_SOURCES rtm/arch_utils_test/*.cpp)
file(GLOB ARCH_UTILS_SOURCES rtm/*.cpp)
set(SOURCES ${ARCH_UTILS_SOURCES} ${RTM_SOURCES})
add_executable(rtm_arch_utils ${SOURCES})
target_link_libraries(rtm_arch_utils LINK_PUBLIC uds boost_system boost_thread prime_api_rtm)

# Benchmark
file(GLOB RTM_BENCHMARK_SOURCES rtm/benchmark/*.cpp)
add_executable(rtm_benchmark ${RTM_BENCHMARK_SOURCES})
target_link_libraries(rtm_benchmark LINK_PUBLIC uds boost_system boost_thread prime_api_rtm arch_utils)

# Linear Regresion with Gradient Descent
message(">> Building LRGD RTM")
find_package(GSL REQUIRED)
if(GSL_FOUND)
	include_directories(rtm/lrgd/include)
	file(GLOB RTM_LRGD_SOURCES rtm/lrgd/src/*.cpp)
	add_executable(rtm_lrgd ${RTM_LRGD_SOURCES})
	target_link_libraries(rtm_lrgd LINK_PUBLIC uds boost_system boost_thread prime_api_rtm arch_utils ${GSL_LIBRARIES})
else(GSL_FOUND)
	message(">> GNU Scientific Library Required for LRGD RTM.")
endif(GSL_FOUND)

# PMC Based
message(">> Building PMC Based RTM")
file(GLOB RTM_PMC_SOURCES rtm/pmc_based/*.cpp)
add_executable(rtm_pmc ${RTM_PMC_SOURCES})
target_link_libraries(rtm_pmc LINK_PUBLIC uds boost_system boost_thread prime_api_rtm arch_utils)

# Thermal
message(">> Building Thermal RTM")
file(GLOB RTM_THERMAL_SOURCES rtm/thermal-rtm/*.cpp rtm/thermal-rtm/*.c)
set(SOURCES ${RTM_THERMAL_SOURCES})
add_executable(rtm_thermal ${SOURCES})
target_link_libraries(rtm_thermal LINK_PUBLIC uds boost_system boost_thread prime_api_rtm arch_utils)

# bb_rtm
message(">> Building BB RTM")
file(GLOB RTM_BB_SOURCES rtm/bb_rtm/*.cpp rtm/thermal-rtm/*.c)
set(SOURCES ${RTM_BB_SOURCES})
add_executable(rtm_bb ${SOURCES})
target_link_libraries(rtm_bb LINK_PUBLIC uds boost_system boost_thread prime_api_rtm arch_utils)

#---------------------------- Devices -----------------------------

if ( ${DEVICE} STREQUAL "Test" )
	# Placeholder for any other device
	message(FATAL_ERROR ">> TEST Placeholder device selected.")

elseif ( ${DEVICE} STREQUAL "XU4" )
	# Placeholder for XU4
	message(FATAL_ERROR ">> XU4 Selected.")
	message(">> Building device file for Odroid XU4")

elseif ( ${DEVICE} STREQUAL "C5SOC" )
	# C5SoC
	message(">> Building device file for c5soc")
	file(GLOB DEV_C5SOC_SOURCES dev/c5soc/*.cpp)
	add_executable(dev ${DEV_C5SOC_SOURCES})
	target_link_libraries(dev LINK_PUBLIC uds boost_system boost_thread prime_api_dev pthread)
	configure_file(dev/c5soc/architecture_c5soc.json architecture_dev.json COPYONLY)
	file(COPY dev/util/kocl/lib/volreg DESTINATION . FILE_PERMISSIONS WORLD_EXECUTE)
	# KOCL
	message(">> Building KOCL")
	find_package(PythonInterp 2.7)
	find_package(PythonLibs 2.7)
	include_directories(${PYTHON_INCLUDE_DIRS})
	add_library(kocl dev/util/kocl/KOCL.cpp)
	include_directories(dev/util/kocl/include/)
	target_link_libraries(kocl LINK_PUBLIC ${PYTHON_LIBRARIES} boost_system boost_thread boost_chrono prime_api_app)
	file(COPY dev/util/kocl/KOCL.py DESTINATION . FILE_PERMISSIONS WORLD_EXECUTE)
	file(COPY dev/util/kocl/lib DESTINATION . FILE_PERMISSIONS WORLD_EXECUTE)

else ()
	# Default to Odroid XU3
	message(">> Building device file for Odroid XU3")
	file(GLOB DEV_ODROID_SOURCES dev/odroid_xu3/*.cpp)
	add_executable(dev ${DEV_ODROID_SOURCES})
	target_link_libraries(dev LINK_PUBLIC uds boost_system boost_thread prime_api_dev pthread)
	configure_file(dev/odroid_xu3/architecture_odroid_xu3.json architecture_dev.json COPYONLY)
endif()

#add_definitions(-DDEBUG_API)
