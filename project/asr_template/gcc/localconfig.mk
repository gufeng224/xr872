#
# project local config options, override the global config options
#

# ----------------------------------------------------------------------------
# board definition
# ----------------------------------------------------------------------------
__PRJ_CONFIG_BOARD = xradio_audio

# ----------------------------------------------------------------------------
# override global config options
# ----------------------------------------------------------------------------
# enable/disable xplayer, default to n
export __CONFIG_XPLAYER := y

# enable/disable XIP, default to y
export __CONFIG_XIP := y

# enable/disable PSRAM, default to n
export __CONFIG_PSRAM := y