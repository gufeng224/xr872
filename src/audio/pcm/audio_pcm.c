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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver/chip/hal_codec.h"
#include "driver/chip/hal_i2s.h"
#include "driver/chip/hal_dmic.h"
#include "driver/chip/hal_i2c.h"
#include "audio/pcm/audio_pcm.h"
#include "audio/manager/audio_manager.h"

#define Oops(fmt, arg...)               printf("[PCM]"fmt, ##arg)
#define AUDIO_OUT_DEV_DEFAULT			AUDIO_OUT_DEV_SPEAKER
#define AUDIO_IN_DEV_DEFAULT			AUDIO_IN_DEV_MAINMIC

#ifndef PCM_ASSERT
#define PCM_WARN_MESSAGE(x)     do {printf(x);} while(0)

#define PCM_ASSERT(message, assertion) do { if(!(assertion)) { \
                                        PCM_WARN_MESSAGE(message); \
                                        return -1; \
                                      } \
                                 } while(0)
#endif

struct play_priv {
	struct pcm_config *config;
	unsigned char     *cache;
	unsigned int      length;
};

struct cap_priv {
	struct pcm_config *config;
};

struct audio_priv {
	struct play_priv  play_priv;
	struct cap_priv   cap_priv;
	OS_Mutex_t        play_lock;
	OS_Mutex_t        write_lock;
	OS_Mutex_t        cap_lock;
};

static struct audio_priv snd_pcm_priv;

#define pcm_lock(n)         OS_MutexLock(&((snd_pcm_priv).n##_lock), OS_WAIT_FOREVER)
#define pcm_unlock(n)       OS_MutexUnlock(&((snd_pcm_priv).n##_lock))
#define pcm_lock_init(n)    OS_MutexCreate(&((snd_pcm_priv).n##_lock))
#define pcm_lock_deinit(n)	OS_MutexDelete(&((snd_pcm_priv).n##_lock))

void* pcm_zalloc(unsigned int size)
{
	void *p;
	if ((p = malloc(size)) != NULL){
		memset(p,0,size);
		return p;
	}
	return NULL;
}

void pcm_free(void *p)
{
	if (p) {
		free(p);
		p = NULL;
	}
	return;
}

unsigned int pcm_get_buffer_size(struct pcm_config *config)
{
	return config->period_count * config->period_size;
}

unsigned int pcm_format_to_bits(enum pcm_format format)
{
	switch (format) {
		case PCM_FORMAT_S32_LE:
		case PCM_FORMAT_S24_LE:
			return 32;
		case PCM_FORMAT_S16_LE:
			return 16;
		default:
			Oops("invalid pcm format...\n");
			return 0;
	};
}

unsigned int pcm_frames_to_bytes(struct pcm_config *config, unsigned int frames)
{
	return frames * config->channels *
		(pcm_format_to_bits(config->format) >> 3);
}

int snd_pcm_write(struct pcm_config *config, unsigned int card, void *data, unsigned int count)
{
	PCM_ASSERT("Invalid card.\n", (card != SOUND_CARD_INTERNAL_DMIC));
	int len = 0, ret = 0, buf_size = 0, size = 0;
	uint8_t *data_ptr = NULL;

	if (pcm_lock(write) != OS_OK) {
		Oops("Obtain write lock err.\n");
		return -1;
	}

	buf_size = pcm_frames_to_bytes(config,pcm_get_buffer_size(config))/2;;
	data_ptr = data;
	size = count;

	struct play_priv *ppriv = &(snd_pcm_priv.play_priv);
	if (ppriv->length != 0) {
		len = buf_size - ppriv->length;
		if (ppriv->cache == NULL) {
			pcm_unlock(write);
			return -1;
		}
		if (len > size) {
			memcpy(ppriv->cache + ppriv->length, data_ptr, size);
			ppriv->length += size;
			pcm_unlock(write);
			return count;
		} else
			memcpy(ppriv->cache + ppriv->length, data_ptr, len);

		HAL_CODEC_Write_DMA(card, (void *)ppriv->cache, buf_size);
		ppriv->length = 0;
	}

	data_ptr = data_ptr +len;
	size -= len;

	ret = HAL_CODEC_Write_DMA(card, data_ptr, size);

	if (ret > 0 && ret != size) {
		data_ptr += ret;
		ppriv->length = size - ret;
		memcpy(ppriv->cache, data_ptr, ppriv->length);
	}else if (ret == size) {
		ppriv->length = 0;
	} else {
		ppriv->length = size;
		memcpy(ppriv->cache, data_ptr, ppriv->length);
	}

#if 0

	#include "fs/fatfs/ff.h"
        FRESULT result;
        FATFS fs;
        FIL file;
        memset(&fs, 0, sizeof(fs));
	unsigned int writenum = 0;
        fs.drv = 1;
        if ((result = f_mount(&fs, "0:/", 1)) != FR_OK) // = "0:" = "0:/" = ""
                printf("failed to mount\n");
        else if ((result = f_open(&file, "0:/5.pcm", FA_OPEN_ALWAYS|FA_READ|FA_WRITE)) != FR_OK)
                printf("[music file]failed to open,%s\n","0:/5.pcm");
        if ((result = f_write(&file, data, count, &writenum)) != FR_OK)
		printf("write failed(%d).\n",result);

	f_close(&file);

#endif
	pcm_unlock(write);
	return count;
}

int snd_pcm_flush(struct pcm_config *config, unsigned int card)
{
	PCM_ASSERT("Invalid card.\n", (card != SOUND_CARD_INTERNAL_DMIC));

	struct play_priv *ppriv = &(snd_pcm_priv.play_priv);
	PCM_ASSERT("Cache is NULL.\n", (ppriv->cache != NULL));
	int buf_size = pcm_frames_to_bytes(config,pcm_get_buffer_size(config));

	pcm_lock(write);

	buf_size = buf_size / 2;
	if (ppriv->length != 0){
		memset(ppriv->cache + ppriv->length, 0, buf_size - ppriv->length);
		HAL_CODEC_Write_DMA(card, (void *)ppriv->cache, buf_size);
		ppriv->length = 0;
	}

	int delay = 0;
	memset(ppriv->cache, 0, buf_size);
	for(delay = 0; delay < 2; delay ++) {
		HAL_CODEC_Write_DMA(card, (void *)ppriv->cache, buf_size);
	}
	pcm_unlock(write);

	return 0;
}

int snd_pcm_read(struct pcm_config *config, unsigned int card, void *data, unsigned int count)
{
	PCM_ASSERT("Invalid card.\n", (card < AUDIO_CARD_NUM));
	void *buf = data;
	int size = 0;
	int  buf_size = pcm_frames_to_bytes(config,pcm_get_buffer_size(config))/2;;
	if (buf_size > count)
		return -1;
	size = (count / buf_size) * buf_size;
	if ((card == SOUND_CARD_EXTERNAL_AUDIOCODEC)
#if (__CONFIG_CHIP_ARCH_VER == 2)
	    || (card == SOUND_CARD_INTERNAL_AUDIOCODEC)
#endif
	)
		return HAL_CODEC_Read_DMA(card, buf, size);
#if (__CONFIG_CHIP_ARCH_VER == 1)
	else if (card == SOUND_CARD_INTERNAL_DMIC)
		return HAL_DMIC_Read_DMA(buf, size);
#endif
	return -1;
}

int snd_pcm_open(struct pcm_config *config, unsigned int card, unsigned int flags)
{
	if ((card == AUDIO_CARD1) && (flags == PCM_OUT)) {
		Oops("Wrong card and flags param.\n");
		return -1;
	}

	mgrctl_ctx *mgr_ctx = aud_mgr_ctx();

	if (card == AUDIO_CARD0
#if (__CONFIG_CHIP_ARCH_VER == 2)
		|| card == AUDIO_CARD2
#endif
	    ) {
		DATA_Param codec_data;
		memset(&codec_data, 0, sizeof(codec_data));

		MANAGER_MUTEX_LOCK(&(mgr_ctx->lock));
		if (mgr_ctx->is_initialize) {
			if (PCM_OUT == flags) {
				if (mgr_ctx->current_dev & AUDIO_OUT_DEV_ALL)
					codec_data.isDevEnable = 1;
			} else {
				if (mgr_ctx->current_dev & AUDIO_IN_DEV_ALL)
					codec_data.isDevEnable = 1;
			}
		}
		MANAGER_MUTEX_UNLOCK(&(mgr_ctx->lock));

		uint32_t bufSize = pcm_frames_to_bytes(config, pcm_get_buffer_size(config));

		codec_data.audio_card = card;
		codec_data.direction = flags;
		codec_data.channels = config->channels;
		codec_data.sampleRate = config->rate;
		codec_data.resolution = config->format;
		codec_data.mixMode = config->mix_mode;
		codec_data.bufSize = bufSize;
		codec_data.audioDev = (PCM_OUT == flags) ? AUDIO_OUT_DEV_DEFAULT : AUDIO_IN_DEV_DEFAULT;
		if (HAL_CODEC_Open(&codec_data) != HAL_OK) {
			Oops("Codec open failed..\n");
			return -1;
		}

		MANAGER_MUTEX_LOCK(&(mgr_ctx->lock));
		if (!codec_data.isDevEnable)
			mgr_ctx->current_dev |= codec_data.audioDev;
		MANAGER_MUTEX_UNLOCK(&(mgr_ctx->lock));

		if (flags == PCM_OUT) {
			if (pcm_lock(play) != OS_OK) {
				Oops("obtain play lock err...\n");
				return -1;
			}
			struct play_priv *ppriv = &(snd_pcm_priv.play_priv);
			ppriv->cache = pcm_zalloc(bufSize/2);
			if (ppriv->cache == NULL) {
				pcm_unlock(play);
				Oops("obtain play cache failed...\n");
				return -1;
			}
			ppriv->length = 0;
			ppriv->config = config;

		} else {
			if (pcm_lock(cap) != OS_OK) {
				Oops("obtain cap lock err...\n");
				return -1;
			}

			struct cap_priv *cpriv = &(snd_pcm_priv.cap_priv);
			cpriv->config = config;
		}

		if (flags == PCM_OUT) {
			if (HAL_CODEC_MUTE_STATUS_Get() == 0) {
				if (codec_data.audioDev == AUDIO_OUT_DEV_SPEAKER)
					HAL_CODEC_Trigger(card, codec_data.audioDev, 1);
			}
		}
		MANAGER_MUTEX_LOCK(&(mgr_ctx->lock));
		if (flags == PCM_OUT)
			mgr_ctx->playback = 1;
		else
			mgr_ctx->record = 1;
		MANAGER_MUTEX_UNLOCK(&(mgr_ctx->lock));
	} else {

		DMIC_DataParam dmic_data;
		memset(&dmic_data, 0, sizeof(dmic_data));
		if (pcm_lock(cap) != OS_OK) {
			Oops("obtain cap lock err...\n");
			return -1;
		}
		switch (config->rate) {
			case 48000:
				dmic_data.sampleRate = DMIC_SR48KHZ;
				break;
			case 44100:
				dmic_data.sampleRate = DMIC_SR44KHZ;
				break;
			case 24000:
				dmic_data.sampleRate = DMIC_SR24KHZ;
				break;
			case 22050:
				dmic_data.sampleRate = DMIC_SR22KHZ;
				break;
			case 12000:
				dmic_data.sampleRate = DMIC_SR12KHZ;
				break;
			case 11025:
				dmic_data.sampleRate = DMIC_SR11KHZ;
				break;
			case 32000:
				dmic_data.sampleRate = DMIC_SR32KHZ;
				break;
			case 16000:
				dmic_data.sampleRate = DMIC_SR16KHZ;
				break;
			case 8000:
				dmic_data.sampleRate = DMIC_SR8KHZ;
				break;

			default:
				break;
		}
		dmic_data.bufSize = pcm_frames_to_bytes(config,pcm_get_buffer_size(config));
		dmic_data.channels = config->channels;
		dmic_data.resolution = (config->format == PCM_FORMAT_S16_LE) ? DMIC_RES16BIT : DMIC_RES24BIT;

#if (__CONFIG_CHIP_ARCH_VER == 1)
		if (HAL_DMIC_Open(&dmic_data) != HAL_OK) {
			Oops("Dmic open failed..\n");
			pcm_unlock(cap);
			return -1;
		}
#endif
		MANAGER_MUTEX_LOCK(&(mgr_ctx->lock));
		mgr_ctx->record = 1;
		MANAGER_MUTEX_UNLOCK(&(mgr_ctx->lock));
	}
	return 0;
}

int snd_pcm_close(unsigned int card, unsigned int flags)
{
	if ((card == AUDIO_CARD1) && (flags == PCM_OUT)) {
		Oops("Wrong card and flags\n");
		return -1;
	}

	int dir = (PCM_OUT == flags) ? PLAYBACK : RECORD;
	mgrctl_ctx *mgr_ctx = aud_mgr_ctx();

	if (card == AUDIO_CARD0
#if (__CONFIG_CHIP_ARCH_VER == 2)
		|| card == AUDIO_CARD2
#endif
		) {
		uint16_t dev;

		MANAGER_MUTEX_LOCK(&(mgr_ctx->lock));
		if (mgr_ctx->is_initialize)
			dev = mgr_ctx->current_dev;
		else
			dev = AUDIO_OUT_DEV_DEFAULT;
		MANAGER_MUTEX_UNLOCK(&(mgr_ctx->lock));

		if (dir == PLAYBACK) {
			if (AUDIO_OUT_DEV_SPEAKER & dev)
				HAL_CODEC_Trigger(card, AUDIO_OUT_DEV_SPEAKER, 0);
		}

		HAL_CODEC_Close(card, dir);

		if (PCM_OUT == flags) {
			pcm_free(snd_pcm_priv.play_priv.cache);
			memset(&(snd_pcm_priv.play_priv), 0, sizeof(struct play_priv));
			pcm_unlock(play);

		} else {
			memset(&(snd_pcm_priv.cap_priv), 0, sizeof(struct cap_priv));
			pcm_unlock(cap);
		}

		MANAGER_MUTEX_LOCK(&(mgr_ctx->lock));
		if (PCM_OUT == flags) {
			mgr_ctx->playback = 0;
			mgr_ctx->current_dev &= ~AUDIO_OUT_DEV_ALL;
		} else {
			mgr_ctx->record = 0;
			mgr_ctx->current_dev &= ~(AUDIO_IN_DEV_MAINMIC | AUDIO_IN_DEV_HEADPHONEMIC);
		}
		MANAGER_MUTEX_UNLOCK(&(mgr_ctx->lock));
	} else {
		MANAGER_MUTEX_LOCK(&(mgr_ctx->lock));
		mgr_ctx->record = 0;
		MANAGER_MUTEX_UNLOCK(&(mgr_ctx->lock));
#if (__CONFIG_CHIP_ARCH_VER == 1)
		HAL_DMIC_Close();
#endif
		memset(&(snd_pcm_priv.cap_priv), 0, sizeof(struct cap_priv));
		pcm_unlock(cap);
	}
	return 0;
}

int snd_pcm_init()
{
	struct audio_priv* audio_priv = &snd_pcm_priv;

	memset(audio_priv, 0, sizeof(*audio_priv));
	pcm_lock_init(play);
	pcm_lock_init(write);
	pcm_lock_init(cap);
	return 0;
}

int snd_pcm_deinit()
{
	pcm_lock_deinit(play);
	pcm_lock_deinit(write);
	pcm_lock_deinit(cap);
	return 0;
}
