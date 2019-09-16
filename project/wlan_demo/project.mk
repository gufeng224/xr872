#
# Rules for building project
#

# ----------------------------------------------------------------------------
# library
# ----------------------------------------------------------------------------
LIBRARY_PATHS := -L$(ROOT_PATH)/lib

# wlan libs
ifeq ($(__CONFIG_WLAN_STA), y)
  ifeq ($(__CONFIG_WLAN_STA_WPS), y)
    LIB_WPA += -lwpas_wps
  else
    LIB_WPA += -lwpas
  endif
endif

ifeq ($(__CONFIG_WLAN_AP), y)
  LIB_WPA += -lhostapd
endif

LIB_WPA += -lwpa

ifeq ($(__CONFIG_ETF), y)
  LIB_WLAN := -lxretf
  ifeq ($(__CONFIG_CHIP_ARCH_VER), 1)
    LIB_WLAN += -lwlan
  endif
else
  LIB_WLAN := -lwlan $(LIB_WPA) $(LIB_WPA) -lnet80211 -lxrwireless -lnet80211
endif

# There are strong and weak symbols in "lchip", it may link to the weak symbol
# as they are statc libraries, so use --whole-archive to solve this problem.
LIBRARIES := -Wl,--whole-archive -lchip -lrom -Wl,--no-whole-archive

# add extra libs from specific project
LIBRARIES += $(PRJ_EXTRA_LIBS)

ifneq ($(__CONFIG_BOOTLOADER), y)

LIBRARIES += -lota
LIBRARIES += -lefpg

ifeq ($(__CONFIG_CHIP_ARCH_VER), 2)
  LIBRARIES += $(LIB_WLAN)
endif

# audio player libs
ifeq ($(__CONFIG_XPLAYER), y)
  LIBRARIES += -lcedarx
  LIBRARIES += -lmp3
  LIBRARIES += -lamr
  LIBRARIES += -lamren
  LIBRARIES += -lwav
  LIBRARIES += -laac
  LIBRARIES += -lcedarx
endif

# network libs
LIBRARIES += -lmqtt
LIBRARIES += -lnopoll
LIBRARIES += -llibwebsockets
LIBRARIES += -lhttpd
LIBRARIES += -lhttpcli
LIBRARIES += -lmbedtls
LIBRARIES += -lsntp
LIBRARIES += -lping
LIBRARIES += -ludhcpd
LIBRARIES += -lsmartlink
LIBRARIES += -lairkiss_aes
LIBRARIES += -lsc_assistant
LIBRARIES += -lxrsys
LIBRARIES += -llwip
LIBRARIES += -lethernetif
LIBRARIES += -lxrsys

# other libs
LIBRARIES += -lcjson
LIBRARIES += -lfs
LIBRARIES += -lconsole
LIBRARIES += -lcomponent
LIBRARIES += -lefpg
LIBRARIES += -lreverb
LIBRARIES += -laudmgr
LIBRARIES += -lpcm
LIBRARIES += -ladt
LIBRARIES += -lutil

endif # __CONFIG_BOOTLOADER

ifeq ($(__CONFIG_BIN_COMPRESS), y)
ifneq ($(__CONFIG_ROM_XZ), y)
  LIBRARIES += -lxz
endif
endif

ifeq ($(__CONFIG_PM), y)
LIBRARIES += -lpm
endif

LIBRARIES += -limage -los

ifeq ($(__CONFIG_OS_FREERTOS), y)
ifneq ($(__CONFIG_ROM_FREERTOS), y)
  LIBRARIES += -lfreertos
endif
endif

LIBRARIES += -lxrc $(LD_SYS_LIBS) -lxrc

# ----------------------------------------------------------------------------
# extra include path
# ----------------------------------------------------------------------------
ifeq ($(__CONFIG_XPLAYER), y)
  CEDARX_INC_DIRS := $(shell find $(ROOT_PATH)/include/cedarx -type d)
  INCLUDE_PATHS += $(foreach dir, $(CEDARX_INC_DIRS), -I$(dir))
endif

INCLUDE_PATHS += -I$(ROOT_PATH)/project

ifneq ($(__PRJ_CONFIG_BOARD),)
  INCLUDE_PATHS += -I$(ROOT_PATH)/project/common/board/$(__PRJ_CONFIG_BOARD)
else
  $(error board is not defined!)
endif

# ----------------------------------------------------------------------------
# include config header for all project
# ----------------------------------------------------------------------------
CC_FLAGS += -include common/prj_conf_opt.h

# ----------------------------------------------------------------------------
# common suffix
# ----------------------------------------------------------------------------
ifeq ($(__CONFIG_XIP), y)
  SUFFIX_XIP := _xip
endif

ifeq ($(__CONFIG_PSRAM), y)
  SUFFIX_PSRAM := _psram
endif

ifeq ($(__CONFIG_BIN_COMPRESS), y)
  SUFFIX_XZ := _xz
endif

# ----------------------------------------------------------------------------
# linker script
# ----------------------------------------------------------------------------
# linker script, maybe override by the specific project
LINKER_SCRIPT_PATH ?= $(ROOT_PATH)/project/linker_script/gcc

ifeq ($(__CONFIG_BOOTLOADER), y)
  LINKER_SCRIPT ?= $(LINKER_SCRIPT_PATH)/bootloader.ld
else
  LINKER_SCRIPT ?= $(LINKER_SCRIPT_PATH)/appos.ld
endif

# linker script generated automatically for building elf file
PROJECT_LD := .project.ld

# ----------------------------------------------------------------------------
# image
# ----------------------------------------------------------------------------
# platform relative path
PLATFORM_RELATIVE_PATH := xradio_v$(__CONFIG_CHIP_ARCH_VER)

ifeq ($(__CONFIG_PLATFORM_FPGA), y)
PLATFORM_RELATIVE_PATH := $(PLATFORM_RELATIVE_PATH)/fpga
endif

ifeq ($(__CONFIG_ETF), y)
PLATFORM_RELATIVE_PATH := $(PLATFORM_RELATIVE_PATH)/etf
endif

# original bin path, files and names
BIN_PATH := $(ROOT_PATH)/bin/$(PLATFORM_RELATIVE_PATH)
BIN_FILES := $(wildcard $(BIN_PATH)/*.bin)
BIN_NAMES := $(notdir $(BIN_FILES))

# copied lib path, files and names
CP_LIB_PATH := $(ROOT_PATH)/lib/$(PLATFORM_RELATIVE_PATH)
CP_LIB_FILES := $(wildcard $(CP_LIB_PATH)/*.a)
CP_LIB_NAMES := $(notdir $(CP_LIB_FILES))

ifeq ($(__CONFIG_ROM), y)
ROM_SYMBOL_NAME := rom_symbol.ld
ROM_SYMBOL_FILE := $(ROOT_PATH)/lib/xradio_v$(__CONFIG_CHIP_ARCH_VER)/$(ROM_SYMBOL_NAME)
endif

ifeq ($(__CONFIG_BIN_COMPRESS), y)

# xz is a tool used to compress bins
XZ_CHECK ?= none
XZ_LZMA2_DICT_SIZE ?= 8KiB
XZ := xz -f -k --no-sparse --armthumb --check=$(XZ_CHECK) \
         --lzma2=preset=6,dict=$(XZ_LZMA2_DICT_SIZE),lc=3,lp=1,pb=1

XZ_DEFAULT_BINS := app.bin
XZ_BINS ?= $(XZ_DEFAULT_BINS)

endif # __CONFIG_BIN_COMPRESS

# output image path
IMAGE_PATH := ../image/$(__CONFIG_CHIP_TYPE)

# $(IMAGE_TOOL) is relative to $(IMAGE_PATH)
IMAGE_TOOL := ../$(ROOT_PATH)/tools/$(MKIMAGE)

# image config file, maybe override by the specific project
# $(IMAGE_CFG_PATH) is relative to $(IMAGE_PATH)
IMAGE_CFG_PATH ?= ../$(ROOT_PATH)/project/image_cfg
IMAGE_CFG ?= $(IMAGE_CFG_PATH)/image.cfg
$(info --------->>>> image_cfg $(IMAGE_CFG))
$(info --------->>>> image_cfg $(IMAGE_CFG_PATH))
# image config file generated automatically for creating image, relative to $(IMAGE_PATH)
PROJECT_IMG_CFG := .image.cfg

# image tool's options to enable/disable OTA
ifeq ($(__CONFIG_OTA), y)
  IMAGE_TOOL_OPT := -O
else
  IMAGE_TOOL_OPT :=
endif

# image name, maybe override by the specific project
IMAGE_NAME ?= xr_system

# ----------------------------------------------------------------------------
# common targets and building rules
# ----------------------------------------------------------------------------
CC_SYMBOLS += $(PRJ_CONFIG_SYMBOLS) $(PRJ_EXTRA_CONFIG_SYMBOLS)

ifeq ($(MDK_DBG_EN), y)
  ELF_EXT = axf
else
  ELF_EXT = elf
endif

ifeq ($(__CONFIG_XIP), y)
  OBJCOPY_R_XIP := -R .xip
  OBJCOPY_J_XIP := -j .xip
endif

ifeq ($(__CONFIG_PSRAM), y)
  OBJCOPY_R_PSRAM := -R .psram_text -R .psram_data -R .psram_bss
  OBJCOPY_J_PSRAM := -j .psram_text -j .psram_data
endif

.PHONY: all $(PROJECT).$(ELF_EXT) objdump size clean lib lib_clean \
	lib_install_clean install image image_clean build build_clean \
	config config_clean

all: $(PROJECT).bin size

$(PROJECT).$(ELF_EXT): $(OBJS)
ifneq ($(__CONFIG_BOOTLOADER), y)
	-@if ls $(CP_LIB_PATH)/*.a > /dev/null 2>&1; then \
		$(Q)$(CP) -t $(ROOT_PATH)/lib $(CP_LIB_FILES); \
	fi
endif
ifeq ($(__CONFIG_ROM), y)
	$(Q)$(CP) -t . $(ROM_SYMBOL_FILE)
endif
	$(Q)$(CC) -E -P -CC $(CC_SYMBOLS) -o $(PROJECT_LD) - < $(LINKER_SCRIPT) && \
	$(Q)$(CC) $(LD_FLAGS) -T$(PROJECT_LD) $(LIBRARY_PATHS) -o $@ $(OBJS) $(LIBRARIES)

%.bin: %.$(ELF_EXT)
	$(Q)$(OBJCOPY) -O binary $(OBJCOPY_R_XIP) $(OBJCOPY_R_PSRAM) $(OBJCOPY_R_EXT) $< $@
ifeq ($(__CONFIG_XIP), y)
	$(Q)$(OBJCOPY) -O binary $(OBJCOPY_J_XIP) $< $(basename $@)$(SUFFIX_XIP).bin
endif
ifeq ($(__CONFIG_PSRAM), y)
	$(Q)$(OBJCOPY) -O binary $(OBJCOPY_J_PSRAM) $< $(basename $@)$(SUFFIX_PSRAM).bin
endif

%.objdump: %.$(ELF_EXT)
	$(Q)$(OBJDUMP) -Sdh $< > $@

objdump: $(PROJECT).objdump

size:
	$(Q)$(SIZE) $(PROJECT).$(ELF_EXT)

clean:
	$(Q)-rm -f $(PROJECT_LD) $(PROJECT).* *.bin $(OBJS) $(DEPS) $(ROM_SYMBOL_NAME)

lib:
	$(Q)$(MAKE) $(S) -C $(ROOT_PATH)/src install

lib_clean:
	$(Q)$(MAKE) $(S) -C $(ROOT_PATH)/src clean

lib_install_clean:
	$(Q)$(MAKE) $(S) -C $(ROOT_PATH)/src install_clean

ifeq ($(__CONFIG_BOOTLOADER), y)

install:
	$(Q)$(CP) $(PROJECT).bin $(BIN_PATH)/boot.bin

build: lib all install

build_clean: clean lib_clean lib_install_clean

else # __CONFIG_BOOTLOADER

install:
	@mkdir -p $(IMAGE_PATH)
	$(Q)$(CP) $(PROJECT).bin $(IMAGE_PATH)/app.bin
ifeq ($(__CONFIG_XIP), y)
	$(Q)$(CP) $(PROJECT)$(SUFFIX_XIP).bin $(IMAGE_PATH)/app$(SUFFIX_XIP).bin
endif
ifeq ($(__CONFIG_PSRAM), y)
	$(Q)$(CP) $(PROJECT)$(SUFFIX_PSRAM).bin $(IMAGE_PATH)/app$(SUFFIX_PSRAM).bin
endif

image: install
	-@if ls $(BIN_PATH)/*.bin > /dev/null 2>&1; then \
		$(Q)$(CP) -t $(IMAGE_PATH) $(BIN_FILES); \
	fi
ifeq ($(__CONFIG_BIN_COMPRESS), y)
	cd $(IMAGE_PATH) && \
	$(Q)$(XZ) $(XZ_BINS)
endif
	cd $(IMAGE_PATH) && \
	chmod a+r *.bin && \
	$(Q)$(CC) -E -P -CC $(CC_SYMBOLS) -o $(PROJECT_IMG_CFG) - < $(IMAGE_CFG) && \
	$(IMAGE_TOOL) $(IMAGE_TOOL_OPT) -c $(PROJECT_IMG_CFG) -o $(IMAGE_NAME).img

image_clean:
	-cd $(IMAGE_PATH) && \
	rm -f $(PROJECT_IMG_CFG) $(BIN_NAMES) app*.bin *.xz *.img

build: lib all image

build_clean: image_clean clean lib_clean lib_install_clean

endif # __CONFIG_BOOTLOADER

config:
	$(Q)cd $(ROOT_PATH) && ./configure.sh

config_clean:
	$(Q)-rm -f $(ROOT_PATH)/.config

# ----------------------------------------------------------------------------
# dependent rules
# ----------------------------------------------------------------------------
DEPS = $(OBJS:.o=.d)
-include $(DEPS)
