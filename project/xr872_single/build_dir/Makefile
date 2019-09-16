#
# Rules for building application
#

# ----------------------------------------------------------------------------
# project local config
# ----------------------------------------------------------------------------
include ../configs/xr872_defconfig.vendor

# ----------------------------------------------------------------------------
# common rules
# ----------------------------------------------------------------------------
OUTPATH := image
#ROOT_DIR:= $(shell pwd)/../../../../..
ROOT_PATH:=../../..
#ROOT_PATH := $(ROOT_DIR)/vendor/xr872
#ROKID_LIB_PATH=../components/rokid_sdk/lib
#VENDOR_CODE:=bin config.mk gcc.mk include lib project src tools
CUR_DIR=$(strip $(shell pwd))
#export CC_DIR=$(CUR_DIR)/../../../../toolchains/gcc-arm-none-eabi-4_9-2015q2/bin
include $(ROOT_PATH)/gcc.mk

# ----------------------------------------------------------------------------
# project and objects
# ----------------------------------------------------------------------------
PROJECT := xr872
INCLUDE_PATHS += -I./main
INCLUDE_PATHS += -I./wifi_api
#INCLUDE_PATHS += -I./fatfs_api
#INCLUDE_PATHS += -I./flash_api
#INCLUDE_PATHS += -I$(ROOT_PATH)/src/smartlink/smartconfig/
#INCLUDE_PATHS += -I../components/rokid_sdk/include

DIRS_IGNORE := ./configs ./image% $(ROOT_PATH)/project/common/board/%
DIRS_ALL := $(shell find . $(ROOT_PATH)/project/common -type d)
DIRS := $(filter-out $(DIRS_IGNORE),$(DIRS_ALL))
DIRS += $(ROOT_PATH)/project/common/board/$(__PRJ_CONFIG_BOARD)

SRCS := $(basename $(foreach dir,$(DIRS),$(wildcard $(dir)/*.[csS])))

OBJS := $(addsuffix .o,$(SRCS))

# extra libs
#PRJ_EXTRA_LIBS :=-L$(ROKID_LIB_PATH) -lrokid-litos --specs=nosys.specs
#PRJ_EXTRA_LIBS +=-L./ailib -lkws -larm_cortexM4l_math
LINKER_SCRIPT := ../appkws.ld
# extra symbols (macros)
#PRJ_EXTRA_CONFIG_SYMBOLS :=

# ----------------------------------------------------------------------------
# override project variables
# ----------------------------------------------------------------------------
# linker script path/file
#   - relative to "./"
#   - define your own "LINKER_SCRIPT_PATH" and/or "LINKER_SCRIPT" to override
#     the default one
# LINKER_SCRIPT_PATH := .
# LINKER_SCRIPT :=

# image config path/file
#   - relative to "../image/xxxxx/", eg. "../image/xr871/"
#   - define your own "IMAGE_CFG_PATH" and/or "IMAGE_CFG" to override the
#     default one
# IMAGE_CFG_PATH := .
IMAGE_CFG := ../../yodalite_image.cfg

# image name, default to xr_system
IMAGE_NAME := yodalite_xr872

# project make rules
include $(PRJ_MAKE_RULES)

unlink:
	-rm -rf $(VENDOR_CODE) $(OUTPATH) components
link:
	-rm -rf $(VENDOR_CODE) $(OUTPATH)
	$(foreach dir,$(VENDOR_CODE),ln -s $(ROOT_PATH)/$(dir) $(dir);)
	ln -s ../$(OUTPATH) $(OUTPATH)
