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
# enable/disable XIP, default to y
export __CONFIG_XIP := y
export __CONFIG_PSRAM := y
export __CONFIG_XPLAYER := y

export __CONFIG_WLAN_STA := y
export __CONFIG_WLAN_STA_WPS := n
export __CONFIG_WLAN_AP := y
