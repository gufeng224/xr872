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

#include "cmd_util.h"
#include "cmd_codec.h"
#include "driver/chip/hal_gpio.h"
#include "driver/chip/hal_codec.h"
#include "driver/chip/codec.h"
#include "fft/fft.h"


#define PCM_BUF_SIZE	(128)

extern AUDIO_CODEC AUDIOCODEC;
FFT_RESULT Cooley_Tukey_FFT(__s32 *data);

unsigned char data_16k16bit2ch_1K[128] = {
	0x01, 0x00, 0x00, 0x00, 0xE1, 0x15, 0xE2, 0x15, 0x6F, 0x28, 0x6E, 0x28, 0xD2, 0x34, 0xD2, 0x34,
	0x2D, 0x39, 0x2D, 0x39, 0xD3, 0x34, 0xD3, 0x34, 0x6E, 0x28, 0x6D, 0x28, 0xE1, 0x15, 0xE1, 0x15,
	0x01, 0x00, 0x00, 0x00, 0x1E, 0xEA, 0x1E, 0xEA, 0x93, 0xD7, 0x92, 0xD7, 0x2D, 0xCB, 0x2D, 0xCB,
	0xD3, 0xC6, 0xD4, 0xC6, 0x2E, 0xCB, 0x2D, 0xCB, 0x92, 0xD7, 0x92, 0xD7, 0x20, 0xEA, 0x1F, 0xEA,
	0x00, 0x00, 0x00, 0x00, 0xE1, 0x15, 0xE1, 0x15, 0x6D, 0x28, 0x6E, 0x28, 0xD3, 0x34, 0xD2, 0x34,
	0x2D, 0x39, 0x2D, 0x39, 0xD3, 0x34, 0xD2, 0x34, 0x6E, 0x28, 0x6E, 0x28, 0xE1, 0x15, 0xE1, 0x15,
	0x00, 0x00, 0x00, 0x00, 0x1E, 0xEA, 0x1E, 0xEA, 0x91, 0xD7, 0x92, 0xD7, 0x2D, 0xCB, 0x2C, 0xCB,
	0xD3, 0xC6, 0xD3, 0xC6, 0x2E, 0xCB, 0x2D, 0xCB, 0x92, 0xD7, 0x92, 0xD7, 0x1E, 0xEA, 0x1E, 0xEA
};


static enum cmd_status cmd_codec_init_exec(char *cmd)
{
	AUDIOCODEC.drv_ops->init();

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_codec_deinit_exec(char *cmd)
{
	AUDIOCODEC.drv_ops->deinit();

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_codec_open_exec(char *cmd)
{
	int cnt;
	uint32_t direction, channels, resolution, sampleRate;

	cnt = cmd_sscanf(cmd, "d=%u c=%u r=%u s=%u", &direction, &channels, &resolution, &sampleRate);
	if (cnt != 4) {
		CMD_ERR("cmd_sscanf return: cnt = %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if (direction != 0 && direction != 1) {
		CMD_ERR("invalid direction %u\n", direction);
		return CMD_STATUS_INVALID_ARG;
	}

	if (channels != 1 && channels != 2) {
		CMD_ERR("invalid channels %u\n", channels);
		return CMD_STATUS_INVALID_ARG;
	}

	if (resolution != 16 && resolution != 24) {
		CMD_ERR("invalid resolution %u\n", resolution);
		return CMD_STATUS_INVALID_ARG;
	}

	switch (sampleRate) {
		case 8000:
		case 16000:
		case 32000:

		case 12000:
		case 24000:
		case 48000:

		case 11025:
		case 22050:
		case 44100:
			break;
		default:
			CMD_ERR("invalid sample rate %u\n", sampleRate);
			return CMD_STATUS_INVALID_ARG;
	}

	CODEC_DataParam codec_param;
	codec_param.direction = direction;
	codec_param.channels = channels;
	codec_param.sampleRate = sampleRate;
	codec_param.resolution = (resolution == 16) ? 0 : 1;
	codec_param.bufSize = PCM_BUF_SIZE;
	if (AUDIOCODEC.drv_ops->open(&codec_param) != HAL_OK) {
		CMD_ERR("Codec open Fail..\n");
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_codec_close_exec(char *cmd)
{
	int cnt;
	uint32_t direction;

	cnt = cmd_sscanf(cmd, "d=%u", &direction);
	if (cnt != 1) {
		CMD_ERR("cmd_sscanf return: cnt = %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if (direction != 0 && direction != 1) {
		CMD_ERR("invalid direction %u\n", direction);
		return CMD_STATUS_INVALID_ARG;
	}

	if (AUDIOCODEC.drv_ops->close(direction) != HAL_OK) {
		CMD_ERR("Codec Close Fail..\n");
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_codec_write_dma_exec(char *cmd)
{
	int cnt;
	uint32_t len;
	uint8_t *buf;
	int32_t size;

	cnt = cmd_sscanf(cmd, "l=%u", &len);
	if (cnt != 1) {
		CMD_ERR("cmd_sscanf return: cnt = %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if (len != PCM_BUF_SIZE && len%PCM_BUF_SIZE != 0) {
		CMD_ERR("invalid len %u\n", len);
		return CMD_STATUS_INVALID_ARG;
	}

	buf = (uint8_t *)cmd_malloc(len * sizeof(uint8_t));
	if (buf == NULL) {
		CMD_ERR("cmd_malloc return NULL.\n");
		return CMD_STATUS_FAIL;
	}

	cmd_write_respond(CMD_STATUS_OK, "OK");

	cmd_raw_mode_enable();

#if 0
	size = cmd_raw_mode_read(buf, (int32_t)len, 10000);
	if (size != (int32_t)len) {
		CMD_ERR("len = %u, but raw mode read size = %d\n", len, size);
		cmd_free(buf);
		cmd_raw_mode_write((uint8_t *)"FAIL", 4);
		cmd_raw_mode_disable();
		return CMD_STATUS_ACKED;
	}
#endif

	size = AUDIOCODEC.drv_ops->write(data_16k16bit2ch_1K, (int32_t)len);
	if (size != (int32_t)len) {
		CMD_ERR("len = %u, but Codec WRITE size = %d\n", len, size);
		cmd_free(buf);
		cmd_raw_mode_write((uint8_t *)"FAIL", 4);
		cmd_raw_mode_disable();
		return CMD_STATUS_ACKED;
	}

	CMD_LOG(1, "write: %s\n", buf);

	cmd_free(buf);
	cmd_raw_mode_write((uint8_t *)"OK", 2);
	cmd_raw_mode_disable();
	return CMD_STATUS_ACKED;
}

static enum cmd_status cmd_codec_read_dma_exec(char *cmd)
{
	int cnt,i;
	uint32_t len;
	uint8_t *buf;
	int32_t size;

	cnt = cmd_sscanf(cmd, "l=%u", &len);
	if (cnt != 1) {
		CMD_ERR("cmd_sscanf return: cnt = %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if (len != PCM_BUF_SIZE && len%PCM_BUF_SIZE != 0) {
		CMD_ERR("invalid len %u\n", len);
		return CMD_STATUS_INVALID_ARG;
	}

	buf = (uint8_t *)cmd_malloc(len * sizeof(uint8_t));
	if (buf == NULL) {
		CMD_ERR("cmd_malloc return NULL.\n");
		return CMD_STATUS_FAIL;
	}
	cmd_memset(buf, 0, len);

	size = AUDIOCODEC.drv_ops->read(buf, (int32_t)len);
	if (size != (int32_t)len) {
		CMD_ERR("len = %u, but Codec read size = %d\n", len, size);
		cmd_free(buf);
		return CMD_STATUS_FAIL;
	}

	cmd_write_respond(CMD_STATUS_OK, "OK");
	CMD_LOG(1, "\n");

	cmd_raw_mode_enable();
	size = cmd_raw_mode_write(buf, (int32_t)len);
	if (size != (int32_t)len) {
		CMD_ERR("len = %u, but raw mode write size = %d\n", len, size);
	}


	/********************************** FFT Analyse **********************************/
	#define FFT_POINTS		1024

	FFT_RESULT fft_result;
	uint16_t *buf_u16 = (uint16_t *)buf;
	s32 *fft_dat = (s32 *)cmd_malloc(FFT_POINTS*4);

	CMD_LOG(1, "\n");
	for(i=0; i<len/2; i++)
		printf("0x%04x ", buf_u16[i]);
	CMD_LOG(1, "\n");

	cmd_memcpy(buf, buf+len/2, len/2);

	if(fft_dat == NULL){
		CMD_ERR("\nMalloc FFT DAT buffer Fail\n\n");
	} else {
		cmd_memset(fft_dat, 0, FFT_POINTS*4);
		//printf("AMIC FFT buf:\n");
		for(i=0; i<FFT_POINTS; i++){
			fft_dat[i] = buf_u16[2*(i%32)];
			//printf("0x%04x ",fft_dat[i]);
		}
		//printf("\n\n");
		fft_result = Cooley_Tukey_FFT(fft_dat);
		printf("\nAMIC FFT Frequency: %f KHz,SNR: %f dB\n\n",fft_result.sig_freq/1000,fft_result.sig_power-fft_result.noise_power);

		cmd_memset(fft_dat, 0, FFT_POINTS*4);

		//printf("LINEIN FFT buf:\n");
		for(i=0; i<FFT_POINTS; i++){
			fft_dat[i] = buf_u16[2*(i%32)+1];
			//printf("0x%04x ",fft_dat[i]);
		}
		//printf("\n\n");
		fft_result = Cooley_Tukey_FFT(fft_dat);
		printf("\nLINEIN FFT Frequency: %f KHz,SNR: %f dB\n\n",fft_result.sig_freq/1000,fft_result.sig_power-fft_result.noise_power);

		cmd_free(fft_dat);
	}
	/*********************************************************************************/

	cmd_free(buf);
	cmd_raw_mode_disable();
	return CMD_STATUS_ACKED;
}

static enum cmd_status cmd_codec_set_route_exec(char *cmd)
{
	int cnt;
	uint32_t route,enable;
	HAL_Status hal_status;
	AUDIO_Device audio_device;

	cnt = cmd_sscanf(cmd, "r=%u e=%u", &route, &enable);
	if (cnt != 2) {
		CMD_ERR("cmd_sscanf return: cnt = %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	switch(route){
		case 0:
			audio_device = AUDIO_IN_DEV_MAINMIC;
			break;
		case 1:
			audio_device = AUDIO_IN_DEV_LINEIN;
			break;
		case 2:
			audio_device = AUDIO_IN_DEV_DIGITALMIC;
			break;
		case 3:
			audio_device = AUDIO_OUT_DEV_SPEAKER;
			break;
		default:
			CMD_ERR("invalid route %u\n", route);
			return CMD_STATUS_INVALID_ARG;
	}

	if(enable != 0 && enable != 1){
		CMD_ERR("invalid enable %u\n", enable);
		return CMD_STATUS_INVALID_ARG;
	}

	hal_status = AUDIOCODEC.ctl_ops->setRoute(audio_device, enable);
	if (hal_status == HAL_OK) {
		return CMD_STATUS_OK;
	} else {
		CMD_ERR("Codec set route Fail, return: hal_status = %d\n", hal_status);
		return CMD_STATUS_FAIL;
	}
}

static enum cmd_status cmd_codec_set_gain_exec(char *cmd)
{
	int cnt;
	uint32_t route,gain;
	HAL_Status hal_status;
	AUDIO_Device audio_device;

	cnt = cmd_sscanf(cmd, "r=%u g=%u", &route, &gain);
	if (cnt != 2) {
		CMD_ERR("cmd_sscanf return: cnt = %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	switch(route){
		case 0:
			audio_device = AUDIO_IN_DEV_MAINMIC;
			break;
		case 1:
			audio_device = AUDIO_IN_DEV_LINEIN;
			break;
		case 3:
			audio_device = AUDIO_OUT_DEV_SPEAKER;
			break;
		case 2:
		default:
			CMD_ERR("invalid route %u\n", route);
			return CMD_STATUS_INVALID_ARG;
	}

	if(route == 3){
		if(gain<0 || gain>31){
			CMD_ERR("invalid gain %u\n", gain);
			return CMD_STATUS_INVALID_ARG;
		}
	} else {
		if(gain<0 || gain>7){
			CMD_ERR("invalid gain %u\n", gain);
			return CMD_STATUS_INVALID_ARG;
		}
	}

	hal_status = AUDIOCODEC.ctl_ops->setVolume(audio_device, gain);
	if (hal_status == HAL_OK) {
		return CMD_STATUS_OK;
	} else {
		CMD_ERR("Codec set gain Fail, return: hal_status = %d\n", hal_status);
		return CMD_STATUS_FAIL;
	}
}

static enum cmd_status cmd_codec_set_adda_exec(char *cmd)
{
	int cnt;
	uint32_t route,enable;
	HAL_Status hal_status;
	AUDIO_Device audio_device;

	cnt = cmd_sscanf(cmd, "r=%u e=%u", &route, &enable);
	if (cnt != 2) {
		CMD_ERR("cmd_sscanf return: cnt = %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	switch(route){
		case 0:
			audio_device = AUDIO_IN_DEV_MAINMIC;
			break;
		case 1:
			audio_device = AUDIO_IN_DEV_LINEIN;
			break;
		case 2:
			audio_device = AUDIO_IN_DEV_DIGITALMIC;
			break;
		default:
			CMD_ERR("invalid route %u\n", route);
			return CMD_STATUS_INVALID_ARG;
	}

	if(enable != 0 && enable != 1){
		CMD_ERR("invalid enable %u\n", enable);
		return CMD_STATUS_INVALID_ARG;
	}

	hal_status = AUDIOCODEC.ctl_ops->ioctl(audio_device, CODEC_CMD_SET_ADDA_DIRECT, enable);
	if (hal_status == HAL_OK) {
		return CMD_STATUS_OK;
	} else {
		CMD_ERR("Codec_set_adda Fail, return: hal_status = %d\n", hal_status);
		return CMD_STATUS_FAIL;
	}
}


static const struct cmd_data g_codec_cmds[] = {
	{ "init",			cmd_codec_init_exec },
	{ "deinit",			cmd_codec_deinit_exec },
	{ "open",			cmd_codec_open_exec },
	{ "close",			cmd_codec_close_exec },
	{ "write-dma",		cmd_codec_write_dma_exec },
	{ "read-dma",		cmd_codec_read_dma_exec },
	{ "set-route",		cmd_codec_set_route_exec},
	{ "set-gain",		cmd_codec_set_gain_exec},
	{ "set-adda",		cmd_codec_set_adda_exec},
};

enum cmd_status cmd_codec_exec(char *cmd)
{
	return cmd_exec(cmd, g_codec_cmds, cmd_nitems(g_codec_cmds));
}

