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
#include "cmd_i2s.h"
#include "driver/chip/hal_gpio.h"
#include "driver/chip/hal_i2s.h"

#define PCM_BUF_SIZE	(256)

#define I2S_LOOP_EN		(1)

#if I2S_LOOP_EN
void I2S_Loop_En(uint8_t en);
//void I2S_RXFIFO_Flush(void);
#endif

/*
 * drv i2s init l=<LoopEn>	*<LoopEn> {0|1}
 */
static enum cmd_status cmd_i2s_init_exec(char *cmd)
{
	int cnt;
	uint32_t loop_en;

	cnt = cmd_sscanf(cmd, "l=%u", &loop_en);
	if (cnt != 1) {
		CMD_ERR("cmd_sscanf return: cnt = %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if (loop_en != 0 && loop_en != 1) {
		CMD_ERR("invalid loop_en %u\n", loop_en);
		return CMD_STATUS_INVALID_ARG;
	}

	I2S_Param i2s_param;
	memset(&i2s_param, 0, sizeof(i2s_param));
	i2s_param.mclkDiv = 1;
	int ret = HAL_I2S_Init(&i2s_param);
	if (ret != HAL_OK) {
		CMD_ERR("I2S Init failed..\n");
		return CMD_STATUS_FAIL;
	}

#if I2S_LOOP_EN
	I2S_Loop_En(loop_en);
#endif

	return CMD_STATUS_OK;
}

/*
 * drv i2s deinit
 */
static enum cmd_status cmd_i2s_deinit_exec(char *cmd)
{
#if I2S_LOOP_EN
	I2S_Loop_En(0);
#endif

	HAL_I2S_DeInit();

	return CMD_STATUS_OK;
}

/*
 * drv i2s open d=<Dirrection> c=<Channels> r=<Resolution> s=<SampleRate>
 *          	*<Dirrection> {0|1}
 *				*<Channels>   {1|2}
 *				*<Resolution> {0|1|2|3}
 *				*<SampleRate> {48000|44100|32000|22050|16000|12000|11025|8000}
 */
static enum cmd_status cmd_i2s_open_exec(char *cmd)
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

	if (channels < 1 || channels > 2) {
		CMD_ERR("invalid channels %u\n", channels);
		return CMD_STATUS_INVALID_ARG;
	}

	switch (sampleRate) {
		case 8000:
			//sampleRate = I2S_SR8K;
			break;
		case 16000:
			//sampleRate = I2S_SR16K;
			break;
		case 32000:
			//sampleRate = I2S_SR32K;
			break;

		case 12000:
			//sampleRate = I2S_SR12K;
			break;
		case 24000:
			//sampleRate = I2S_SR24K;
			break;
		case 48000:
			//sampleRate = I2S_SR48K;
			break;

		case 11025:
			//sampleRate = I2S_SR11K;
			break;
		case 22050:
			//sampleRate = I2S_SR22K;
			break;
		case 44100:
			//sampleRate = I2S_SR44K;
			break;

		default:
			CMD_ERR("Invalid sample rate(%u) failed...\n",sampleRate);
			return HAL_INVALID;
	}

	switch(resolution){
		case 8:
			resolution = I2S_SR8BIT;
			break;
		case 12:
			resolution = I2S_SR12BIT;
			break;
		case 16:
			resolution = I2S_SR16BIT;
			break;
		case 20:
			resolution = I2S_SR20BIT;
			break;
		case 24:
			resolution = I2S_SR24BIT;
			break;
		case 28:
			resolution = I2S_SR28BIT;
			break;
		case 32:
			resolution = I2S_SR32BIT;
			break;

		default:
			CMD_ERR("Invalid resolution(%u) failed...\n",resolution);
			return HAL_INVALID;
	}

	I2S_DataParam i2s_data;
	i2s_data.direction = direction;
	i2s_data.channels = channels;
	i2s_data.resolution = resolution;
	i2s_data.sampleRate = sampleRate;
	i2s_data.bufSize = PCM_BUF_SIZE;
	if (HAL_I2S_Open(&i2s_data) != HAL_OK) {
		CMD_ERR("I2S open failed..\n");
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}

/*
 * drv i2s close d=<Dirrection>
 *				 *<Dirrection> {0|1}
 *
 */
static enum cmd_status cmd_i2s_close_exec(char *cmd)
{
	int cnt;
	uint32_t direction;
	HAL_Status hal_status;

	cnt = cmd_sscanf(cmd, "d=%u", &direction);
	if (cnt != 1) {
		CMD_ERR("cmd_sscanf return: cnt = %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if (direction != 0 && direction != 1) {
		CMD_ERR("invalid direction %u\n", direction);
		return CMD_STATUS_INVALID_ARG;
	}

	hal_status = HAL_I2S_Close(direction);
	if (hal_status == HAL_OK) {
		return CMD_STATUS_OK;
	} else {
		CMD_ERR("HAL_I2S_Close return: hal_status = %d\n", hal_status);
		return CMD_STATUS_FAIL;
	}
}

/*
 * drv i2s write-dma l=<Length>
 *					 if exec "drv i2s init l=1": *<Length> : {128}
 *					 else <Length> : {128, 256, 384, 512...}
 */
static enum cmd_status cmd_i2s_write_dma_exec(char *cmd)
{
	int cnt;
	uint16_t input_wait_time;
	uint32_t len;
	uint8_t *buf;
	int32_t size;

	cnt = cmd_sscanf(cmd, "l=%u t=%hu", &len,&input_wait_time);
	if (cnt != 2) {
		CMD_ERR("cmd_sscanf return: cnt = %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if (len != (PCM_BUF_SIZE/2) && (len % PCM_BUF_SIZE) != 0) {
		CMD_ERR("invalid len %u\n", len);
		return CMD_STATUS_INVALID_ARG;
	}

	if(input_wait_time <= 0){
		input_wait_time = 10;
	}

	buf = (uint8_t *)cmd_malloc(len * sizeof(uint8_t));
	if (buf == NULL) {
		CMD_ERR("cmd_malloc return NULL.\n");
		return CMD_STATUS_FAIL;
	}

	cmd_write_respond(CMD_STATUS_OK, "OK");
	CMD_LOG(1, "\nwrite buf:\n");

	cmd_raw_mode_enable();
	size = cmd_raw_mode_read(buf, (int32_t)len, 1000*input_wait_time);
	if (size != (int32_t)len) {
		CMD_ERR("len = %u, but raw mode read size = %d\n", len, size);
		cmd_free(buf);
		cmd_raw_mode_write((uint8_t *)"FAIL", 4);
		cmd_raw_mode_disable();
		return CMD_STATUS_ACKED;
	}

	size = HAL_I2S_Write_DMA(buf, (int32_t)len);
	if (size != (int32_t)len) {
		CMD_ERR("len = %u, but I2S WRITE size = %d\n", len, size);
		cmd_free(buf);
		cmd_raw_mode_write((uint8_t *)"FAIL", 4);
		cmd_raw_mode_disable();
		return CMD_STATUS_ACKED;
	}
	if (I2S_LOOP_EN)
		//I2S_RXFIFO_Flush();

	//cmd_msleep(10000);

	CMD_LOG(1, "write: %s\n", buf);

	cmd_free(buf);
	cmd_raw_mode_write((uint8_t *)"OK", 2);
	cmd_raw_mode_disable();
	return CMD_STATUS_ACKED;
}

/*
 * drv i2s read-dma l=<Length>
 *					if exec "drv i2s init l=1": *<Length> : {128}
 *					else <Length> : {128, 256, 384, 512...}
 */
static enum cmd_status cmd_i2S_read_dma_exec(char *cmd)
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

	if (len != (PCM_BUF_SIZE/2) && (len % PCM_BUF_SIZE) != 0) {
		CMD_ERR("invalid len %u\n", len);
		return CMD_STATUS_INVALID_ARG;
	}

	buf = (uint8_t *)cmd_malloc(len * sizeof(uint8_t));
	if (buf == NULL) {
		CMD_ERR("cmd_malloc return NULL.\n");
		return CMD_STATUS_FAIL;
	}
	cmd_memset(buf, 0, len);

	size = HAL_I2S_Read_DMA(buf, (int32_t)len);
	if (size != (int32_t)len) {
		CMD_ERR("len = %u, but I2S read size = %d\n", len, size);
		cmd_free(buf);
		return CMD_STATUS_FAIL;
	}

	cmd_write_respond(CMD_STATUS_OK, "OK");
	CMD_LOG(1, "\nread buf:\n");

	cmd_raw_mode_enable();
	size = cmd_raw_mode_write(buf, (int32_t)len);
	if (size != (int32_t)len) {
		CMD_ERR("len = %u, but raw mode write size = %d\n", len, size);
	}
	CMD_LOG(1, "\n");

	/*cmd_msleep(2000);
	uint32_t i =0;
	for(i=0; i<len; i++)
		printf("0x%x ", buf[i]);
	CMD_LOG(1, "\n");*/

	cmd_free(buf);
	cmd_raw_mode_disable();
	return CMD_STATUS_ACKED;
}

static const struct cmd_data g_i2s_cmds[] = {
	{ "init",			cmd_i2s_init_exec },
	{ "deinit",			cmd_i2s_deinit_exec },
	{ "open",			cmd_i2s_open_exec },
	{ "close",			cmd_i2s_close_exec },
	{ "write-dma",		cmd_i2s_write_dma_exec },
	{ "read-dma",		cmd_i2S_read_dma_exec },
};

enum cmd_status cmd_i2s_exec(char *cmd)
{
	return cmd_exec(cmd, g_i2s_cmds, cmd_nitems(g_i2s_cmds));
}
