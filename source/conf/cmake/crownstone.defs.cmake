# This file can load variables and set them in multiple ways. 
#
# 1. From the configuration files there are variables that have to be added to compiler as definitions. For example, 
#    -DBOARD_PCA10040 for the (Nordic) mesh code. 
# 2. There are variables that need to be known to CMake, however, these are already loaded. The only reason to call
#    SET(...) would be to also add them to the cache. This has often undesired side effects.
#
# TODO: A lot of those definitions can be removed. Both from the bottom SET() list as well as precompiler definitions.


ADD_DEFINITIONS("-D${DEVICE}")
ADD_DEFINITIONS("-D${NRF_DEVICE} -D${NRF_DEVICE_TYPE} -D${NRF_DEVICE_FAMILY} -D${NRF_DEVICE_SERIES}")
ADD_DEFINITIONS("-MMD -DEPD_ENABLE_EXTRA_RAM")
ADD_DEFINITIONS("-DUSE_RENDER_CONTEXT -DSYSCALLS")
ADD_DEFINITIONS("-DUSING_FUNC")

# TODO: Is this used?
ADD_DEFINITIONS("-DDEBUG_NRF")

LIST(APPEND CUSTOM_DEFINITIONS, TEMPERATURE)

ADD_DEFINITIONS("-DBLE_STACK_SUPPORT_REQD")

# We will assume the softdevice is always present
ADD_DEFINITIONS("-DSOFTDEVICE_PRESENT")

MESSAGE(STATUS "crownstone.defs.cmake: Build type: ${CMAKE_BUILD_TYPE}")
IF(CMAKE_BUILD_TYPE MATCHES "Debug")
	ADD_DEFINITIONS("-DDEBUG")
ELSEIF(CMAKE_BUILD_TYPE MATCHES "Release")
	MESSAGE(STATUS "Release build")
ELSEIF(CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
ELSEIF(CMAKE_BUILD_TYPE MATCHES "MinSizeRel")
ELSE()
	MESSAGE(FATAL_ERROR "There should be a CMAKE_BUILD_TYPE defined (and known)")
ENDIF()

# The bluetooth name is not optional
IF(DEFINED BLUETOOTH_NAME)
	ADD_DEFINITIONS("-DBLUETOOTH_NAME=${BLUETOOTH_NAME}")
ELSE()
	MESSAGE(FATAL_ERROR "We require a BLUETOOTH_NAME in CMakeBuild.config (5 characters or less), i.e. \"Crown\" (with quotes)")
ENDIF()

IF(NOT DEFINED CS_SERIAL_ENABLED OR CS_SERIAL_ENABLED STREQUAL "0")
	MESSAGE(STATUS "Crownstone serial disabled")
ELSE()
	MESSAGE(STATUS "Crownstone serial enabled")
ENDIF()

# Copied from an example makefile.
ADD_DEFINITIONS("-DBLE_STACK_SUPPORT_REQD")

IF(DEFINED NORDIC_HARDWARE_BOARD)
	IF(NORDIC_HARDWARE_BOARD MATCHES "PCA10040")
		ADD_DEFINITIONS("-DBOARD_PCA10040")
	ENDIF()
	IF(NORDIC_HARDWARE_BOARD MATCHES "PCA10100")
		ADD_DEFINITIONS("-DBOARD_PCA10100")
	ENDIF()
ELSE()
	MESSAGE(WARNING "Assuming NORDIC_HARDWARE_BOARD MATCHES to be PCA10040")
	ADD_DEFINITIONS("-DBOARD_PCA10040")
ENDIF()

# Apply patches (depends on the family)
IF(NRF_DEVICE_FAMILY MATCHES NRF52)
	MESSAGE(STATUS "Apply PAN 74 workaround")
	ADD_DEFINITIONS("-DNRF52_PAN_74")
ENDIF()

#ADD_DEFINITIONS("-DCONFIG_GPIO_AS_PINRESET") TODO: only define this if board is pca10040?
ADD_DEFINITIONS("-DFLOAT_ABI_HARD")
ADD_DEFINITIONS("-DMBEDTLS_CONFIG_FILE=\"nrf_crypto_mbedtls_config.h\"")
ADD_DEFINITIONS("-DNRF_CRYPTO_MAX_INSTANCE_COUNT=1")
ADD_DEFINITIONS("-DNRF_SD_BLE_API_VERSION=${SOFTDEVICE_MAJOR}")
ADD_DEFINITIONS("-DS${SOFTDEVICE_SERIES}")
ADD_DEFINITIONS("-DSOFTDEVICE_PRESENT")
ADD_DEFINITIONS("-DSWI_DISABLE0")

# Encryption parameters
ADD_DEFINITIONS("-DuECC_ENABLE_VLI_API=0")
ADD_DEFINITIONS("-DuECC_OPTIMIZATION_LEVEL=3")
ADD_DEFINITIONS("-DuECC_SQUARE_FUNC=0")
ADD_DEFINITIONS("-DuECC_SUPPORT_COMPRESSED_POINT=0")
ADD_DEFINITIONS("-DuECC_VLI_NATIVE_LITTLE_ENDIAN=1")

# For mesh SDK
IF (BUILD_MESHING)
	ADD_DEFINITIONS("-DCONFIG_APP_IN_CORE")
	SET(EXPERIMENTAL_INSTABURST_ENABLED OFF)
	IF (EXPERIMENTAL_INSTABURST_ENABLED)
		ADD_DEFINITIONS("-DEXPERIMENTAL_INSTABURST_ENABLED")
	ENDIF()
	SET(MESH_MEM_BACKEND "stdlib")
	# HF timer peripheral index to allocate for bearer handler. E.g. if set to 2, NRF_TIMER2 will be used. Must be a literal number.
	ADD_DEFINITIONS("-DBEARER_ACTION_TIMER_INDEX=3")
ENDIF()

# Overwrite sdk config options
ADD_DEFINITIONS("-DUSE_APP_CONFIG")

# Pass variables in defined in the configuration file to the compiler
ADD_DEFINITIONS("-DNRF5_DIR=${NRF5_DIR}")
ADD_DEFINITIONS("-DNORDIC_SDK_VERSION=${NORDIC_SDK_VERSION}")

# Add info on softdevice (TODO: almost nothing is used of this)
ADD_DEFINITIONS("-DSOFTDEVICE_SERIES=${SOFTDEVICE_SERIES}")
ADD_DEFINITIONS("-DSOFTDEVICE_MAJOR=${SOFTDEVICE_MAJOR}")
ADD_DEFINITIONS("-DSOFTDEVICE_MINOR=${SOFTDEVICE_MINOR}")
ADD_DEFINITIONS("-DSOFTDEVICE=${SOFTDEVICE}")

# Add options to control logs / serial / verbosity (TODO: does not need to be macros)
ADD_DEFINITIONS("-DSERIAL_VERBOSITY=${SERIAL_VERBOSITY}")
ADD_DEFINITIONS("-DCS_SERIAL_NRF_LOG_ENABLED=${CS_SERIAL_NRF_LOG_ENABLED}")
ADD_DEFINITIONS("-DCS_SERIAL_BOOTLOADER_NRF_LOG_ENABLED=${CS_SERIAL_BOOTLOADER_NRF_LOG_ENABLED}")
ADD_DEFINITIONS("-DCS_UART_BINARY_PROTOCOL_ENABLED=${CS_UART_BINARY_PROTOCOL_ENABLED}")

# UICR options (across firmware and bootloader, needs separate cs_SharedConfig.h file if removed here)
ADD_DEFINITIONS("-DUICR_DFU_INDEX=${UICR_DFU_INDEX}")
ADD_DEFINITIONS("-DUICR_BOARD_INDEX=${UICR_BOARD_INDEX}")

# Add support for mesh
ADD_DEFINITIONS("-DBUILD_MESHING=${BUILD_MESHING}")

# Add support for microapps
ADD_DEFINITIONS("-DBUILD_MICROAPP_SUPPORT=${BUILD_MICROAPP_SUPPORT}")

# Add memory options for microapp
ADD_DEFINITIONS("-DRAM_MICROAPP_AMOUNT=${RAM_MICROAPP_AMOUNT}")

# Add twi driver
ADD_DEFINITIONS("-DBUILD_TWI=${BUILD_TWI}")

# Add gpiote driver
ADD_DEFINITIONS("-DBUILD_GPIOTE=${BUILD_GPIOTE}")

# Add support for in-network localization
ADD_DEFINITIONS("-DBUILD_MESH_TOPOLOGY_RESEARCH=${BUILD_MESH_TOPOLOGY_RESEARCH}")
ADD_DEFINITIONS("-DBUILD_CLOSEST_CROWNSTONE_TRACKER=${BUILD_CLOSEST_CROWNSTONE_TRACKER}")

# Build for memory usage test
ADD_DEFINITIONS("-DBUILD_MEM_USAGE_TEST=${BUILD_MEM_USAGE_TEST}")

# Publish options as CMake options as well
SET(NRF5_DIR                                    "${NRF5_DIR}"                       CACHE STRING "Nordic SDK Directory" FORCE)
SET(NORDIC_SDK_VERSION                          "${NORDIC_SDK_VERSION}"             CACHE STRING "Nordic SDK Version" FORCE)

SET(SOFTDEVICE_SERIES                           "${SOFTDEVICE_SERIES}"              CACHE STRING "SOFTDEVICE_SERIES" FORCE)
SET(SOFTDEVICE_MAJOR                            "${SOFTDEVICE_MAJOR}"               CACHE STRING "SOFTDEVICE_MAJOR" FORCE)
SET(SOFTDEVICE_MINOR                            "${SOFTDEVICE_MINOR}"               CACHE STRING "SOFTDEVICE_MINOR" FORCE)
SET(SOFTDEVICE                                  "${SOFTDEVICE}"                     CACHE STRING "SOFTDEVICE" FORCE)

SET(SERIAL_VERBOSITY                            "${SERIAL_VERBOSITY}"               CACHE STRING "SERIAL_VERBOSITY" FORCE)
SET(CS_SERIAL_NRF_LOG_ENABLED                   "${CS_SERIAL_NRF_LOG_ENABLED}"      CACHE STRING "CS_SERIAL_NRF_LOG_ENABLED" FORCE)

