#
# Rules for building application
#

# ----------------------------------------------------------------------------
# project local config
# ----------------------------------------------------------------------------
include localconfig.mk

# ----------------------------------------------------------------------------
# common rules
# ----------------------------------------------------------------------------
ROOT_PATH := ../../..

include $(ROOT_PATH)/gcc.mk

# ----------------------------------------------------------------------------
# project and objects
# ----------------------------------------------------------------------------
PROJECT := speech_template

INCLUDE_PATHS += -I$(ROOT_PATH)/project/$(PROJECT)

DIRS_IGNORE := ../gcc% ../image% $(ROOT_PATH)/project/common/board/%
DIRS_ALL := $(shell find .. $(ROOT_PATH)/project/common -type d)
DIRS := $(filter-out $(DIRS_IGNORE),$(DIRS_ALL))
DIRS += $(ROOT_PATH)/project/common/board/$(__PRJ_CONFIG_BOARD)

SRCS := $(basename $(foreach dir,$(DIRS),$(wildcard $(dir)/*.[csS])))

OBJS := $(addsuffix .o,$(SRCS))
#PRJ_EXTRA_LIBS += -L. -lkws -larm_cortexM4l_math
PRJ_EXTRA_LIBS += -L. -lkws -larm_cortexM4l_math
# extra libs
#LIBRARY_PATHS += -L$(ROOT_PATH)/project/$(PROJECT)/tutuclear/

# extra symbols (macros)
# PRJ_EXTRA_CONFIG_SYMBOLS :=

# ----------------------------------------------------------------------------
# override project variables
# ----------------------------------------------------------------------------
# linker script path/file
#   - relative to "./"
#   - define your own "LINKER_SCRIPT_PATH" and/or "LINKER_SCRIPT" to override
#     the default one
LINKER_SCRIPT_PATH := .
LINKER_SCRIPT := template.ld

# image config path/file
#   - relative to "../image/$(__CONFIG_CHIP_TYPE)/", eg. "../image/xr871/"
#   - define your own "IMAGE_CFG_PATH" and/or "IMAGE_CFG" to override the
#     default one
#IMAGE_CFG_PATH := .
#IMAGE_CFG := image.cfg
IMAGE_CFG_PATH := ../../image/xr872
IMAGE_CFG := $(IMAGE_CFG_PATH)/image.cfg

# image name, default to xr_system
# IMAGE_NAME :=

# project make rules
include $(PRJ_MAKE_RULES)
