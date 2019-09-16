/**
  * @file  codec.h
  * @author  XRADIO IOT WLAN Team
  */

/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DRIVER_CHIP_CODEC_H_
#define _DRIVER_CHIP_CODEC_H_

#include "driver/chip/hal_codec.h"
#include "driver/chip/hal_i2s.h"

#ifdef __cplusplus
 extern "C" {
#endif

typedef struct {
	uint8_t level;
	uint8_t reg_val;
} Volume;

typedef enum {
	HAL_CODEC_INIT      = 0,
	HAL_CODEC_DEINIT
} CODEC_Req;

/**
  * @brief codec data init structure definition
  */
typedef struct {
	uint32_t        sampleRate;   /*!< Specifies the sampling rate of the transmitted data.    */
	uint8_t         direction;    /*!< Specifies the direction of the transmitted data.    */
	uint8_t  		resolution;   /*!< Specifies the sampling accuracy of the transmitted data.    */
	uint8_t         channels;     /*!< Specifies the number of channels to transmit data.    */
	uint32_t        bufSize;      /*!< Specifies the buffer size of the transmitted data.    */
} CODEC_DataParam;

/**
  * @brief DAI pll/clock/format configuration, all optional..
  *        Called by codec drivers.
  */
struct codec_dai_ops {
	int32_t (*startup)(AUDIO_Device device);
	int32_t (*setPll)(const DAI_FmtParam *fmtParam);
	int32_t (*setClkdiv)(const DAI_FmtParam *fmtParam,uint32_t sampleRate);
	int32_t (*setFormat)(const DAI_FmtParam *fmtParam);
	int32_t (*shutDown)(bool playOn, bool recordOn);
};

/**
  * @brief Stream path/volume/trigger configuration, all optional.
  *        Called by codec drivers.
  */
struct codec_ctl_ops {
	int32_t (*setRoute)(AUDIO_Device device, CODEC_DevState state);
	int32_t (*setVolume)(AUDIO_Device dev, uint8_t volume);
	int32_t (*setTrigger)(AUDIO_Device dev, uint8_t on);
	int32_t (*setEqScene)(uint8_t scene);
	int32_t (*ioctl)(AUDIO_Device dev, CODEC_ControlCmd cmd, uint32_t arg);
};

/**
  * @brief Stream path/volume/trigger configuration, all optional.
  *        Called by codec drivers.
  */
struct codec_ops {
	int32_t (*setPower)(CODEC_Req req, void *arg);
	int32_t (*setSysClk)(CODEC_Req req, void *arg);
	int32_t (*setInitParam)(CODEC_Req req, void *arg);
	int32_t (*jackDetect)(CODEC_Req req, void *arg);
};

/**
  * @brief PCM driver, all optional.
  *        Called by codec drivers.
  */
struct codec_drv_ops {
	int32_t (*open)(CODEC_DataParam *param);
	int32_t (*close)(uint32_t dir);
	int32_t (*write)(uint8_t *buf, uint32_t size);
	int32_t (*read)(uint8_t *buf, uint32_t size);
	int32_t (*init)(void);
	int32_t (*deinit)(void);
};

/**
  * @brief  Codec (external) info structure definition
  */
typedef struct {
	AUDIO_CODEC_Type            type;     	 /*!< type of codec    */
	uint8_t                     RegLength;    /*!< I2C registers length    */
	uint8_t                     RegValLength; /*!< I2C registers values length    */
	uint8_t                     i2cId;
	uint8_t                     i2cAddr;
	hw_write                    write;             /*!< I2C write function    */
	hw_read                     read;              /*!< I2C read function    */
	const struct codec_ops     *ops;         /*!< Init the low level hardware : GPIO, CLOCK, POWER, JACK...etc     */
	const struct codec_dai_ops *dai_ops;     /*!< Init codec pcm interface     */
	const struct codec_ctl_ops *ctl_ops;     /*!< Config stream volume and path    */
#if (__CONFIG_CHIP_ARCH_VER == 2)
	struct codec_drv_ops 	   *drv_ops;
#endif
} CODEC_INFO, *CODECP;

/**
  * @brief  Codec (internal) info structure definition
  */
typedef struct {
	struct codec_ctl_ops     *ctl_ops;
	struct codec_drv_ops 	   *drv_ops;
} AUDIO_CODEC;

struct CodecProbeParam {
	uint8_t   i2cId;
};

struct CodecProbeResult {
	AUDIO_CODEC_Type type;
	uint8_t   i2cId;
	uint8_t   i2cAddr;
};

struct CodecCreateParam {
	uint8_t   i2cId;
	uint8_t   i2cAddr;
	hw_write  write;
	hw_read   read;
};

struct CodecCreatorS {
	CODEC_INFO *(*create)(struct CodecCreateParam *);
	HAL_Status (*probe)(struct CodecProbeParam *, struct CodecProbeResult *);
	HAL_Status (*destroy)(CODEC_INFO *);
};

int32_t snd_soc_read(uint32_t reg);
int32_t snd_soc_write(uint32_t reg, uint32_t reg_val);
int32_t snd_soc_update_bits(uint32_t reg, uint32_t mask,uint32_t value);

int32_t snd_reg_read(CODEC_INFO *codec, uint32_t reg);
int32_t snd_reg_write(CODEC_INFO *codec, uint32_t reg, uint32_t reg_val);
int32_t snd_reg_update_bits(CODEC_INFO *codec, uint32_t reg, uint32_t mask, uint32_t value);

#define CODEC_I2C_REG_LENGTH8               1
#define CODEC_I2C_REGVAL_LENGTH8            1
#define CODEC_I2C_REGVAL_LENGTH16           2

#define MCLK1                               1
#define BCLK1                               2

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_CHIP_CODEC_H_ */
