#
# global config options
#

# ----------------------------------------------------------------------------
# config options
# ----------------------------------------------------------------------------
include $(ROOT_PATH)/chip.mk

# redefine int32_t to signed int, but not signed long
__CONFIG_LIBC_REDEFINE_GCC_INT32_TYPE ?= y

# support printf float variables
__CONFIG_LIBC_PRINTF_FLOAT ?= y

# support scanf float variables
__CONFIG_LIBC_SCANF_FLOAT ?= y

# wrap standard input/output/error functions
__CONFIG_LIBC_WRAP_STDIO ?= y

# heap managed by stdlib
__CONFIG_MALLOC_USE_STDLIB ?= y

# trace heap memory usage and error when using malloc, free, etc.
__CONFIG_MALLOC_TRACE ?= n

# os
__CONFIG_OS_FREERTOS ?= y

# lwIP
#   - y: lwIP 1.4.1, support IPv4 stack only
#   - n: lwIP 2.x.x, support dual IPv4/IPv6 stack
__CONFIG_LWIP_V1 ?= y

# mbuf implementation mode
#   - mode 0: continuous memory allocated from heap
#   - mode 1: continuous memory allocated from lwip pbuf
__CONFIG_MBUF_IMPL_MODE ?= 0

# wlan
__CONFIG_WLAN ?= y

# wlan station mode
__CONFIG_WLAN_STA ?= y

# wlan hostap mode
__CONFIG_WLAN_AP ?= y

# wlan monitor mode
__CONFIG_WLAN_MONITOR ?= y

# wps of wlan station mode
__CONFIG_WLAN_STA_WPS ?= n

# wlan ETF test
__CONFIG_ETF ?= n

# wlan ETF test using ETF command line interface
#   - y: using ETF command line interface
#   - n: using ETF GUI exec on PC
__CONFIG_ETF_CLI ?= n

# Wi-Fi Certification of WFA
__CONFIG_WIFI_CERTIFIED ?= y

# XIP
__CONFIG_XIP ?= y

# enable section attribute macros "__xip_xxx", eg. __xip_text
ifeq ($(__CONFIG_XIP), y)
  __CONFIG_SECTION_ATTRIBUTE_XIP ?= y
else
  __CONFIG_SECTION_ATTRIBUTE_XIP ?= n
endif

# enable section attribute macros "__nonxip_xxx", eg. __nonxip_text
ifeq ($(__CONFIG_XIP), y)
  __CONFIG_SECTION_ATTRIBUTE_NONXIP ?= y
else
  __CONFIG_SECTION_ATTRIBUTE_NONXIP ?= n
endif

# enable section attribute macros "__sram_xxx", eg. __sram_text
__CONFIG_SECTION_ATTRIBUTE_SRAM ?= y

# enable section attribute macros "__psram_xxx", eg. __psram_text
__CONFIG_SECTION_ATTRIBUTE_PSRAM ?= n

# rom
ifeq ($(__CONFIG_CHIP_ARCH_VER), 1)
  __CONFIG_ROM ?= n
else
  __CONFIG_ROM ?= y
endif

# rom of FreeRTOS
ifeq ($(__CONFIG_ROM), y)
  __CONFIG_ROM_FREERTOS ?= n
else
  __CONFIG_ROM_FREERTOS ?= n
endif

# rom of xz
ifeq ($(__CONFIG_ROM), y)
  __CONFIG_ROM_XZ ?= y
else
  __CONFIG_ROM_XZ ?= n
endif

# bin compression
__CONFIG_BIN_COMPRESS ?= n

# enable/disable bootloader, y to enable bootloader and disable some features
__CONFIG_BOOTLOADER ?= n

# power manager
__CONFIG_PM ?= y

# psram
__CONFIG_PSRAM ?= n
ifeq ($(__CONFIG_PSRAM), y)
  __CONFIG_PSRAM_CHIP_OPI32 ?= y
else
  __CONFIG_PSRAM_CHIP_OPI32 ?= n
endif
__CONFIG_PSRAM_CHIP_OPI64 ?= n
__CONFIG_PSRAM_CHIP_SQPI ?= n

# OTA
__CONFIG_OTA ?= n

# xplayer
__CONFIG_XPLAYER ?= n

# ----------------------------------------------------------------------------
# config symbols
# ----------------------------------------------------------------------------
ifeq ($(__CONFIG_LIBC_REDEFINE_GCC_INT32_TYPE), y)
  CONFIG_SYMBOLS += -D__CONFIG_LIBC_REDEFINE_GCC_INT32_TYPE
endif

ifeq ($(__CONFIG_LIBC_PRINTF_FLOAT), y)
  CONFIG_SYMBOLS += -D__CONFIG_LIBC_PRINTF_FLOAT
endif

ifeq ($(__CONFIG_LIBC_SCANF_FLOAT), y)
  CONFIG_SYMBOLS += -D__CONFIG_LIBC_SCANF_FLOAT
endif

ifeq ($(__CONFIG_LIBC_WRAP_STDIO), y)
  CONFIG_SYMBOLS += -D__CONFIG_LIBC_WRAP_STDIO
endif

ifeq ($(__CONFIG_MALLOC_USE_STDLIB), y)
  CONFIG_SYMBOLS += -D__CONFIG_MALLOC_USE_STDLIB
endif

ifeq ($(__CONFIG_MALLOC_TRACE), y)
  CONFIG_SYMBOLS += -D__CONFIG_MALLOC_TRACE
endif

ifeq ($(__CONFIG_OS_FREERTOS), y)
  CONFIG_SYMBOLS += -D__CONFIG_OS_FREERTOS
endif

ifeq ($(__CONFIG_LWIP_V1), y)
  CONFIG_SYMBOLS += -D__CONFIG_LWIP_V1
endif

CONFIG_SYMBOLS += -D__CONFIG_MBUF_IMPL_MODE=$(__CONFIG_MBUF_IMPL_MODE)

ifeq ($(__CONFIG_WLAN), y)
  CONFIG_SYMBOLS += -D__CONFIG_WLAN
else
  __CONFIG_WLAN_STA := n
  __CONFIG_WLAN_AP := n
  __CONFIG_WLAN_MONITOR := n
  __CONFIG_WLAN_STA_WPS := n
endif

ifeq ($(__CONFIG_WLAN_STA), y)
  CONFIG_SYMBOLS += -D__CONFIG_WLAN_STA
endif

ifeq ($(__CONFIG_WLAN_AP), y)
  CONFIG_SYMBOLS += -D__CONFIG_WLAN_AP
endif

ifeq ($(__CONFIG_WLAN_MONITOR), y)
  CONFIG_SYMBOLS += -D__CONFIG_WLAN_MONITOR
endif

ifeq ($(__CONFIG_WLAN_STA_WPS), y)
  CONFIG_SYMBOLS += -D__CONFIG_WLAN_STA_WPS
endif

ifeq ($(__CONFIG_ETF), y)
  CONFIG_SYMBOLS += -D__CONFIG_ETF
endif

ifeq ($(__CONFIG_ETF_CLI), y)
  CONFIG_SYMBOLS += -D__CONFIG_ETF_CLI
endif

ifeq ($(__CONFIG_WIFI_CERTIFIED), y)
  CONFIG_SYMBOLS += -D__CONFIG_WIFI_CERTIFIED
endif

ifeq ($(__CONFIG_XIP), y)
  CONFIG_SYMBOLS += -D__CONFIG_XIP
endif

ifeq ($(__CONFIG_SECTION_ATTRIBUTE_XIP), y)
  CONFIG_SYMBOLS += -D__CONFIG_SECTION_ATTRIBUTE_XIP
endif

ifeq ($(__CONFIG_SECTION_ATTRIBUTE_NONXIP), y)
  CONFIG_SYMBOLS += -D__CONFIG_SECTION_ATTRIBUTE_NONXIP
endif

ifeq ($(__CONFIG_SECTION_ATTRIBUTE_SRAM), y)
  CONFIG_SYMBOLS += -D__CONFIG_SECTION_ATTRIBUTE_SRAM
endif

ifeq ($(__CONFIG_SECTION_ATTRIBUTE_PSRAM), y)
  CONFIG_SYMBOLS += -D__CONFIG_SECTION_ATTRIBUTE_PSRAM
endif

ifeq ($(__CONFIG_ROM), y)
  CONFIG_SYMBOLS += -D__CONFIG_ROM
endif

ifeq ($(__CONFIG_ROM_FREERTOS), y)
  CONFIG_SYMBOLS += -D__CONFIG_ROM_FREERTOS
endif

ifeq ($(__CONFIG_ROM_XZ), y)
  CONFIG_SYMBOLS += -D__CONFIG_ROM_XZ
endif

ifeq ($(__CONFIG_BIN_COMPRESS), y)
  CONFIG_SYMBOLS += -D__CONFIG_BIN_COMPRESS
endif

ifeq ($(__CONFIG_BOOTLOADER), y)
  CONFIG_SYMBOLS += -D__CONFIG_BOOTLOADER
endif

ifeq ($(__CONFIG_PM), y)
  CONFIG_SYMBOLS += -D__CONFIG_PM
endif

ifeq ($(__CONFIG_PSRAM), y)
  CONFIG_SYMBOLS += -D__CONFIG_PSRAM
endif

ifeq ($(__CONFIG_PSRAM_CHIP_SQPI), y)
  CONFIG_SYMBOLS += -D__CONFIG_PSRAM_CHIP_SQPI
endif

ifeq ($(__CONFIG_PSRAM_CHIP_OPI32), y)
  CONFIG_SYMBOLS += -D__CONFIG_PSRAM_CHIP_OPI32
endif

ifeq ($(__CONFIG_PSRAM_CHIP_OPI64), y)
  CONFIG_SYMBOLS += -D__CONFIG_PSRAM_CHIP_OPI64
endif

ifeq ($(__CONFIG_OTA), y)
  CONFIG_SYMBOLS += -D__CONFIG_OTA
endif

ifeq ($(__CONFIG_XPLAYER), y)
  CONFIG_SYMBOLS += -D__CONFIG_XPLAYER
endif
