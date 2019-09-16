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

#include "driver/chip/codec.h"

int32_t snd_reg_read(CODEC_INFO *codec, uint32_t reg)
{
	int16_t ret = 0;
	uint8_t val[5];
	uint16_t *reg_val;

	ret = codec->read(codec->i2cId, codec->i2cAddr, reg, codec->RegLength, val, codec->RegValLength);
	if (ret != codec->RegValLength)
		return -1;

	reg_val = (uint16_t *)val;

	switch (codec->RegValLength) {
	case CODEC_I2C_REGVAL_LENGTH8:
		return ((*reg_val)&0xFF);
	case CODEC_I2C_REGVAL_LENGTH16:
		return ((((*reg_val)&0xFF) << 8) |(((*reg_val)&0xFF00)>>8));
	default:
		return -1;
	}
}

int32_t snd_reg_write(CODEC_INFO *codec, uint32_t reg, uint32_t reg_val)
{
	uint32_t ret;
	uint8_t val[5];
	int8_t regValLength = 0;

	switch (codec->RegValLength) {
	case CODEC_I2C_REGVAL_LENGTH8:
		regValLength = 1;
		val[0] = reg_val & 0xFF;
		break;
	case CODEC_I2C_REGVAL_LENGTH16:
		regValLength = 2;
		val[0] = (reg_val & 0xFF00) >> 8;
		val[1] = reg_val & 0xFF;
		break;
	default:
		return -1;
	}

	ret = codec->write(codec->i2cId, codec->i2cAddr, reg, codec->RegValLength, val, regValLength);
	if (ret != regValLength)
		return -1;
	else
		return 0;
}

/**
  * @internal
  * @brief update codec register bits.
  * @param reg: codec register.
  * @param mask: register mask.
  * @param value: new value.
  * @retval Returns 1 for change, 0 for no change, or negative error code.
  */
int32_t snd_reg_update_bits(CODEC_INFO *codec, uint32_t reg, uint32_t mask, uint32_t value)
{
	bool change;
	uint32_t old, val;
	int ret;
	ret = snd_reg_read(codec, reg);
	if (ret < 0)
		return ret;
	old = ret;
	val = (old & ~mask) | (value & mask);
	change = old != val;
	if (change)
		ret = snd_reg_write(codec, reg, val);
	if (ret < 0)
		return ret;
	return change;
}

