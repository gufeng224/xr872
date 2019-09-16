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

#include "../hal_base.h"
#include "driver/chip/codec.h"
#include "hal_ac107.h"

#define AC107_SHOW_REG_VAL         0
#define AC107_DBG_ON               1

static CODEC_INFO *codec;


#if (AC107_DBG_ON == 1)
#define AC107_DEBUG(fmt, arg...)   HAL_LOG(AC107_DBG_ON, "[AC107] "fmt, ##arg)
#else
#define AC107_DEBUG(fmt, arg...)
#endif
#define AC107_ERROR(fmt, arg...)   HAL_LOG(1, "[AC107] "fmt, ##arg)

#define SYSCLK_11M                  (11289600U)
#define SYSCLK_12M                  (12288000U)
#define AC107_SYSCLK_USE_PLL_EN     1

#define AC107_ADC_PATTERN_SEL		0

#define AC107_ENCODING_FMT			0
#define AC107_ENCODING_EN			0
#define AC107_LRCK_PERIOD			32
#define AC107_PDM_EN				1

#define AC107_REG_LENGTH        CODEC_I2C_REG_LENGTH8
#define AC107_REG_VALUE_LENGTH  CODEC_I2C_REGVAL_LENGTH8

static CODEC_MIC_Type mic_type = CODEC_MIC_ANALOG;

struct real_val_to_reg_val {
	unsigned int real_val;
	unsigned int reg_val;
};

#if AC107_SYSCLK_USE_PLL_EN
struct sample_rate_to_pdmclk {
	unsigned int sample_rate;
	unsigned int pdmclk;
};

struct pll_div {
	uint32_t freq_in;
	uint32_t freq_out;
	uint32_t m1;
	uint32_t m2;
	uint32_t n;
	uint32_t k1;
	uint32_t k2;
};
#endif

static const struct real_val_to_reg_val ac107_sample_rate[] = {
	{8000,  0},
	{11025, 1},
	{12000, 2},
	{16000, 3},
	{22050, 4},
	{24000, 5},
	{32000, 6},
	{44100, 7},
	{48000, 8},
};

#if AC107_SYSCLK_USE_PLL_EN
static const struct sample_rate_to_pdmclk ac107_pdmclk[] = {
	{8000,  1024000},
	{11025, 1411200},
	{12000, 1536000},
	{16000, 2048000},
	{22050, 2822400},
	{24000, 3072000},
	{32000, 2048000},
	{44100, 2822400},
	{48000, 3072000},
};

//FOUT =(FIN * N) / [(M1+1) * (M2+1)*(K1+1)*(K2+1)] ;	M1[0,31],  M2[0,1],  N[0,1023],  K1[0,31],  K2[0,1]
static const struct pll_div ac107_pll_div[] = {
	{400000,   12288000, 0,  0, 983,  15, 1},    //<out: 12.2875M>
	{512000,   12288000, 0,  0, 960,  19, 1},    //24576000/48
	{768000,   12288000, 0,  0, 640,  19, 1},    //24576000/32
	{800000,   12288000, 0,  0, 768,  24, 1},
	{1024000,  12288000, 0,  0, 480,  19, 1},    //24576000/24
	{1600000,  12288000, 0,  0, 384,  24, 1},
	{2048000,  12288000, 0,  0, 240,  19, 1},    //24576000/12
	{3072000,  12288000, 0,  0, 160,  19, 1},    //24576000/8
	{4096000,  12288000, 0,  0, 120,  19, 1},    //24576000/6
	{6000000,  12288000, 4,  0, 512,  24, 1},
	{6144000,  12288000, 1,  0, 160,  19, 1},    //24576000/4
	{12000000, 12288000, 9,  0, 512,  24, 1},
	{13000000, 12288000, 12, 0, 639,  25, 1},    //<out: 12.2885M>
	{15360000, 12288000, 9,  0, 320,  19, 1},
	{16000000, 12288000, 9,  0, 384,  24, 1},
	{19200000, 12288000, 11, 0, 384,  24, 1},
	{19680000, 12288000, 15, 1, 999,  24, 1},    //<out: 12.2877M>
	{24000000, 12288000, 9,  0, 256,  24, 1},

	{400000,   11289600, 0,  0, 1016, 17, 1},    //<out: 11.2889M>
	{512000,   11289600, 0,  0, 882,  19, 1},
	{768000,   11289600, 0,  0, 588,  19, 1},
	{800000,   11289600, 0,  0, 508,  17, 1},    //<out: 11.2889M>
	{1024000,  11289600, 0,  0, 441,  19, 1},
	{1600000,  11289600, 0,  0, 254,  17, 1},    //<out: 11.2889M>
	{2048000,  11289600, 1,  0, 441,  19, 1},
	{3072000,  11289600, 0,  0, 147,  19, 1},
	{4096000,  11289600, 3,  0, 441,  19, 1},
	{6000000,  11289600, 1,  0, 143,  18, 1},    //<out: 11.2895M>
	{6144000,  11289600, 1,  0, 147,  19, 1},
	{12000000, 11289600, 3,  0, 143,  18, 1},    //<out: 11.2895M>
	{13000000, 11289600, 12, 0, 429,  18, 1},    //<out: 11.2895M>
	{15360000, 11289600, 14, 0, 441,  19, 1},
	{16000000, 11289600, 24, 0, 882,  24, 1},
	{19200000, 11289600, 4,  0, 147,  24, 1},
	{19680000, 11289600, 13, 1, 771,  23, 1},    //<out: 11.28964M>
	{24000000, 11289600, 24, 0, 588,  24, 1},

	{12288000, 12288000, 9,  0, 400,  19, 1},    //24576000/2
	{11289600, 11289600, 9,  0, 400,  19, 1},    //22579200/2

	{24576000/1,   12288000, 9, 0, 200, 19, 1},  //24576000
	{24576000/16,  12288000, 0, 0, 320, 19, 1},  //1536000
	{24576000/64,  12288000, 0, 0, 640, 9,  1},  //384000
	{24576000/96,  12288000, 0, 0, 960, 9,  1},  //256000
	{24576000/128, 12288000, 0, 0, 512, 3,  1},  //192000
	{24576000/176, 12288000, 0, 0, 880, 4,  1},  //140000
	{24576000/192, 12288000, 0, 0, 960, 4,  1},  //128000

	{22579200/1,   11289600, 9, 0, 200, 19, 1},  //22579200
	{22579200/4,   11289600, 4, 0, 400, 19, 1},  //5644800
	{22579200/16,  11289600, 0, 0, 320, 19, 1},  //1411200
	{22579200/64,  11289600, 0, 0, 640, 9,  1},  //352800
	{22579200/96,  11289600, 0, 0, 960, 9,  1},  //235200
	{22579200/128, 11289600, 0, 0, 512, 3,  1},  //176400
	{22579200/176, 11289600, 0, 0, 880, 4,  1},  //128290
	{22579200/192, 11289600, 0, 0, 960, 4,  1},  //117600

	{22579200/6,   11289600, 2, 0, 360, 19, 1},  //3763200
	{22579200/8,   11289600, 0, 0, 160, 19, 1},  //2822400
	{22579200/12,  11289600, 0, 0, 240, 19, 1},  //1881600
	{22579200/24,  11289600, 0, 0, 480, 19, 1},  //940800
	{22579200/32,  11289600, 0, 0, 640, 19, 1},  //705600
	{22579200/48,  11289600, 0, 0, 960, 19, 1},  //470400
};
#endif

#if AC107_PDM_EN
static void AC107_output_config_PDM(CODEC_INFO *codec)
{
	/* PDM Interface Latch ADC1 data on rising clock edge. Latch ADC2 data on falling clock edge, PDM Enable */
	snd_reg_update_bits(codec, PDM_CTRL, 0x1<<PDM_TIMING | 0x1<<PDM_EN, 0x1<<PDM_TIMING | !!AC107_PDM_EN<<PDM_EN);
}
#else
static void AC107_output_config_I2S(CODEC_INFO *codec)
{
	/*** I2S Common Config ***/
	snd_reg_update_bits(codec, I2S_CTRL, 0x1<<SDO_EN, 0x1<<SDO_EN);	     /* SDO enable */
	snd_reg_update_bits(codec, I2S_BCLK_CTRL, 0x1<<EDGE_TRANSFER, 0x0<<EDGE_TRANSFER);	/* SDO drive data and SDI sample data at the different BCLK edge */
	snd_reg_update_bits(codec, I2S_LRCK_CTRL1, 0x3<<LRCK_PERIODH, ((AC107_LRCK_PERIOD-1) >> 8)<<LRCK_PERIODH);
	snd_reg_write(codec, I2S_LRCK_CTRL2, (uint8_t)(AC107_LRCK_PERIOD-1));    /* config LRCK period */
	/* Encoding mode format select 0~N-1, Encoding mode enable, Turn to hi-z state (TDM) when not transferring slot */
	snd_reg_update_bits(codec, I2S_FMT_CTRL1, 0x1<<ENCD_FMT | 0x1<<ENCD_SEL | 0x1<<TX_SLOT_HIZ | 0x1<<TX_STATE,\
                  !!AC107_ENCODING_FMT<<ENCD_FMT | !!AC107_ENCODING_EN<<ENCD_SEL | 0x0<<TX_SLOT_HIZ | 0x1<<TX_STATE);
	/* 0x36=0x60: TX MSB first, SDOUT normal, PCM frame type, Linear PCM Data Mode */
	snd_reg_update_bits(codec, I2S_FMT_CTRL3, 0x1<<TX_MLS | 0x1<<SDOUT_MUTE | 0x1<<LRCK_WIDTH | 0x3<<TX_PDM,\
                  0x0<<TX_MLS | 0x0<<SDOUT_MUTE | 0x0<<LRCK_WIDTH | 0x0<<TX_PDM);
	snd_reg_update_bits(codec, I2S_TX_CHMP_CTRL1, 0xff, 0xaa);	/* 0x3c=0xaa: TX CH1/3/5/7 map to adc1, TX CH2/4/6/8 map to adc2 */
	snd_reg_update_bits(codec, I2S_TX_CHMP_CTRL2, 0xff, 0xaa);	/* 0x3d=0xaa: TX CH9/11/13/15 map to adc1, TX CH10/12/14/16 map to adc2 */
}
#endif

static void AC107_output_config(CODEC_INFO *codec)
{
#if AC107_PDM_EN
	AC107_output_config_PDM(codec);
#else
	AC107_output_config_I2S(codec);
#endif
}

static void AC107_input_config_Analog(CODEC_INFO *codec)
{
	/*** Analog voltage enable ***/
	snd_reg_write(codec, PWR_CTRL1, 0x80); /* 0x01=0x80: VREF Enable */
	snd_reg_write(codec, PWR_CTRL2, 0x55); /* 0x02=0x55: MICBIAS1&2 Enable */

	/*** ADCs analog PGA gain Config***/
	//snd_reg_update_bits(codec, ANA_ADC1_CTRL3, 0x1f<<RX1_PGA_GAIN_CTRL, AC107_PGA_GAIN<<RX1_PGA_GAIN_CTRL);
	//snd_reg_update_bits(codec, ANA_ADC2_CTRL3, 0x1f<<RX2_PGA_GAIN_CTRL, AC107_PGA_GAIN<<RX2_PGA_GAIN_CTRL);

	/*** ADCs analog global Enable ***/
	snd_reg_update_bits(codec, ANA_ADC1_CTRL5, 0x1<<RX1_GLOBAL_EN, 0x1<<RX1_GLOBAL_EN);
	snd_reg_update_bits(codec, ANA_ADC2_CTRL5, 0x1<<RX2_GLOBAL_EN, 0x1<<RX2_GLOBAL_EN);

#ifdef WAIT_MABIA_STABLE
	/* wait for voltage stable */
	HAL_MSleep(5);
#endif

	/* VREF Fast Start-up Disable */
	snd_reg_update_bits(codec, PWR_CTRL1, 0x1<<VREF_FSU_DISABLE, 0x1<<VREF_FSU_DISABLE);
}

static void AC107_input_config_DMIC(CODEC_INFO *codec)
{
	snd_reg_update_bits(codec, DMIC_EN, 0x1, 0x1);	/* DMIC Enable */
}

static void AC107_hw_init_common(CODEC_INFO *codec)
{
	/*** SYSCLK Config ***/
	snd_reg_write(codec, MOD_CLK_EN, 0x07);    /* 0x21=0x07: Module clock enable<I2S, ADC digital,  ADC analog> */
	snd_reg_write(codec, MOD_RST_CTRL, 0x03);  /* 0x22=0x03: Module reset de-asserted<I2S, ADC digital> */

	/*** ADC DIG part Config ***/
	snd_reg_update_bits(codec, ADC_DIG_EN, 0x7, 0x7);   /* 0x61=0x07: Digital part globe enable, ADCs digital part enable */

	/* ADC pattern select */
	snd_reg_write(codec, HPF_EN, !AC107_ADC_PATTERN_SEL*0x03);
	snd_reg_update_bits(codec, ADC_DIG_DEBUG, 0x7<<ADC_PTN_SEL, (AC107_ADC_PATTERN_SEL&0x7)<<ADC_PTN_SEL);

	snd_reg_write(codec, I2S_PADDRV_CTRL, 0x57);

#if AC107_SHOW_REG_VAL
	uint8_t val;
	uint8_t i;
	for (i = 0; i < 176; i++) {
		val = snd_reg_read(codec, i);
		if (!(i % 4)) {
			AC107_DEBUG("\n");
		}
		AC107_DEBUG("[reg_0x%02x: 0x%02x] ", i, val);
	}
	AC107_DEBUG("\n");
#endif
}

/*
 * Set main mic as the current input device.
 */
static void AC107_SetMainMic(CODEC_INFO *codec)
{
	AC107_DEBUG("Route(cap): main mic..\n");

	AC107_input_config_Analog(codec);
	AC107_output_config(codec);
	AC107_hw_init_common(codec);
}

/*
 * Set digital mic as the current input device.
 */
static void AC107_SetDigitalMic(CODEC_INFO *codec)
{
	AC107_DEBUG("Route(cap): digital mic..\n");

	AC107_input_config_DMIC(codec);
	AC107_output_config(codec);
	AC107_hw_init_common(codec);
}


//************************************ [codec_ctl_ops] ************************************//
/*
 * Set audio output/input device.
 */
static int32_t AC107_SetRoute(AUDIO_Device device, CODEC_DevState state)
{
	switch (device) {
	case AUDIO_IN_DEV_MAINMIC:
		if(mic_type == CODEC_MIC_ANALOG)
			AC107_SetMainMic(codec);
		else
			AC107_SetDigitalMic(codec);
		break;
	default:
		break;
	}
	return HAL_OK;
}

static int32_t AC107_SetVolume(AUDIO_Device dev, uint8_t volume)
{
	return HAL_OK;
}

static int32_t AC107_SetTrigger(AUDIO_Device dev, uint8_t on)
{
	return HAL_OK;
}

static int32_t AC107_Ioctl(AUDIO_Device dev, CODEC_ControlCmd cmd, uint32_t arg)
{
	int ret = -1;

	return ret;
}

static int32_t AC107_SetEqScene(uint8_t scene)
{
	return HAL_OK;
}


//************************************ [codec_dai_ops] ************************************//
static int32_t AC107_SetPll(const DAI_FmtParam *fmtParam)
{
	return HAL_OK;
}

static int32_t AC107_SetClkdiv(const DAI_FmtParam *fmtParam,uint32_t sampleRate)
{
	unsigned int i;

	/* AC107 set sample rate */
	for (i = 0; i < HAL_ARRAY_SIZE(ac107_sample_rate); i++) {
		if (ac107_sample_rate[i].real_val == sampleRate) {
			snd_reg_update_bits(codec, ADC_SPRC, 0xf<<ADC_FS_I2S, ac107_sample_rate[i].reg_val<<ADC_FS_I2S);
			break;
		}
	}

#if AC107_SYSCLK_USE_PLL_EN
	unsigned int freq_in, freq_out, m1, m2, n, k1, k2;

	freq_in = freq_out = m1 = m2 = n = k1 = k2 = 0;

	for (i = 0; i < HAL_ARRAY_SIZE(ac107_pdmclk); i++) {
		if (ac107_pdmclk[i].sample_rate == sampleRate) {
			freq_in = ac107_pdmclk[i].pdmclk;
			break;
		}
	}

	if (sampleRate % 1000) {    // 11025/22050/44100
		freq_out = 11289600;
	} else {                    // 8000/16000/32000/12000/24000/48000
		freq_out = 12288000;
	}

	//FOUT =(FIN * N) / [(M1+1) * (M2+1)*(K1+1)*(K2+1)] ;
	for (i = 0; i < HAL_ARRAY_SIZE(ac107_pll_div); i++) {
		if (ac107_pll_div[i].freq_in == freq_in && ac107_pll_div[i].freq_out == freq_out) {
			m1 = ac107_pll_div[i].m1;
			m2 = ac107_pll_div[i].m2;
			n  = ac107_pll_div[i].n;
			k1 = ac107_pll_div[i].k1;
			k2 = ac107_pll_div[i].k2;
			AC107_DEBUG("AC107 PLL freq_in match:%u, freq_out:%u\n", freq_in, freq_out);
			break;
		}
	}

	if (i == HAL_ARRAY_SIZE(ac107_pll_div)) {
		AC107_DEBUG("AC107 don't match PLLCLK freq_in and freq_out table\n");
		return HAL_ERROR;
	}

	//Config PLL DIV param M1/M2/N/K1/K2
	snd_reg_update_bits(codec, PLL_CTRL2, 0x1f<<PLL_PREDIV1 | 0x1<<PLL_PREDIV2, m1<<PLL_PREDIV1 | m2<<PLL_PREDIV2);
	snd_reg_update_bits(codec, PLL_CTRL3, 0x3<<PLL_LOOPDIV_MSB, (n>>8)<<PLL_LOOPDIV_MSB);
	snd_reg_update_bits(codec, PLL_CTRL4, 0xff<<PLL_LOOPDIV_LSB, (unsigned char)n<<PLL_LOOPDIV_LSB);
	snd_reg_update_bits(codec, PLL_CTRL5, 0x1f<<PLL_POSTDIV1 | 0x1<<PLL_POSTDIV2, k1<<PLL_POSTDIV1 | k2<<PLL_POSTDIV2);

	//PLL module enable
	snd_reg_update_bits(codec, PLL_LOCK_CTRL, 0x1<<PLL_LOCK_EN, 0x1<<PLL_LOCK_EN);                          //PLL CLK lock enable
	snd_reg_update_bits(codec, PLL_CTRL1, 0x1<<PLL_EN | 0x1<<PLL_COM_EN, 0x1<<PLL_EN | 0x1<<PLL_COM_EN);   //PLL Common voltage Enable, PLL Enable

	//PLLCLK Enable, PLLCLK Source Select PDMCLK, SYSCLK Source Select PLL, SYSCLK Enable
	snd_reg_update_bits(codec, SYSCLK_CTRL, 0x1<<PLLCLK_EN | 0x3<<PLLCLK_SRC | 0x3<<SYSCLK_SRC | 0x1<<SYSCLK_EN,
	                           0x1<<PLLCLK_EN | PLLCLK_SRC_PDMCLK<<PLLCLK_SRC | SYSCLK_SRC_PLL<<SYSCLK_SRC | 0x1<<SYSCLK_EN);
#else
	//SYSCLK Source Select MCLK, SYSCLK Enable
	snd_reg_update_bits(codec, SYSCLK_CTRL, 0x3<<SYSCLK_SRC | 0x1<<SYSCLK_EN, SYSCLK_SRC_MCLK<<SYSCLK_SRC | 0x1<<SYSCLK_EN);
#endif

	return HAL_OK;
}

static int32_t AC107_SetFormat(const DAI_FmtParam *fmtParam)
{
	uint8_t slot_width, word_width;
	slot_width = fmtParam->slotWidth/4 - 1;
	word_width = fmtParam->wordWidth/4 - 1;

	if (slot_width > 7 || word_width > 7)
		return HAL_INVALID;

	/* Slot width & SampleResolution */
	snd_reg_update_bits(codec, I2S_FMT_CTRL2, 0x7<<SLOT_WIDTH_SEL | 0x7<<SAMPLE_RESOLUTION, slot_width<<SLOT_WIDTH_SEL | word_width<<SAMPLE_RESOLUTION);

	/* I2S Slave mode */
	snd_reg_update_bits(codec, I2S_CTRL, 0x3<<LRCK_IOEN, 0x0<<LRCK_IOEN);

	/* I2S format */
	snd_reg_update_bits(codec, I2S_FMT_CTRL1, 0x3<<MODE_SEL | 0x1<<TX_OFFSET, LEFT_JUSTIFIED_FORMAT<<MODE_SEL | 0x1<<TX_OFFSET);
	snd_reg_update_bits(codec, I2S_FMT_CTRL3, 0x3<<SEXT, TRANSFER_ZERO_AFTER<<SEXT);

	/* LRCK&BCLK Polarity */
	snd_reg_update_bits(codec, I2S_BCLK_CTRL,  0x1<<BCLK_POLARITY, BCLK_NORMAL_DRIVE_N_SAMPLE_P<<BCLK_POLARITY);
	snd_reg_update_bits(codec, I2S_LRCK_CTRL1, 0x1<<LRCK_POLARITY, LRCK_LEFT_LOW_RIGHT_HIGH<<LRCK_POLARITY);

	/* I2S TX channels config */
	snd_reg_write(codec, I2S_TX_CTRL1, 2-1);
	snd_reg_write(codec, I2S_TX_CTRL2, (uint8_t)0x3);
	snd_reg_write(codec, I2S_TX_CTRL3, 0x3>>8);

	/* AC107 TX enable, Globle enable */
	snd_reg_update_bits(codec, I2S_CTRL, 0x1<<TXEN | 0x1<<GEN, 0x1<<TXEN | 0x1<<GEN);

	return HAL_OK;
}
/*
static int32_t AC107_RegCtl(CODEC_REGISTER_CMD cmd, struct regParam *param)
{
	switch (cmd) {
	case CODEC_REGISTER_CMD_READ:
		param->reg_val = snd_reg_read(codec, param->reg);
		break;
	case CODEC_REGISTER_CMD_WRITE:
		snd_reg_write(codec, param->reg, param->reg_val);
		break;
	default:
		break;
	}
	return 0;
}

static int32_t AC107_Attribute()
{
	return (AUDIO_IN_DEV_MAINMIC | AUDIO_IN_DEV_HEADPHONEMIC);
}
*/
static int32_t AC107_ShutDown(bool playOn, bool recordOn)
{
	uint8_t val[4];

	/* AC107 I2S Globle disable */
	snd_reg_update_bits(codec, I2S_CTRL, 0x1<<GEN, 0x0<<GEN);

	/* AC107 analog%digital gain save */
	val[0] = snd_reg_read(codec, ADC1_DVOL_CTRL);
	val[1] = snd_reg_read(codec, ADC2_DVOL_CTRL);
	val[2] = snd_reg_read(codec, ANA_ADC1_CTRL3);
	val[3] = snd_reg_read(codec, ANA_ADC2_CTRL3);

	/* Soft reset */
	AC107_DEBUG("AC107 reset all register to their default value\n\n");
	snd_reg_write(codec, CHIP_AUDIO_RST, 0x12);

	/* AC107 analog%digital gain restore */
	snd_reg_write(codec, ADC1_DVOL_CTRL, val[0]);
	snd_reg_write(codec, ADC2_DVOL_CTRL, val[1]);
	snd_reg_write(codec, ANA_ADC1_CTRL3, val[2]);
	snd_reg_write(codec, ANA_ADC2_CTRL3, val[3]);

	return HAL_OK;
}


//************************************ [codec_ops] ************************************//
static int32_t AC107_SetPower(CODEC_Req req, void *arg)
{
	return HAL_OK;
}

static int32_t AC107_SetSysClk(CODEC_Req req, void *arg)
{
	return HAL_OK;
}

static int32_t AC107_SetInitParam(CODEC_Req req, void *arg)
{
	snd_reg_write(codec, CHIP_AUDIO_RST, 0x12);

	if (req == HAL_CODEC_INIT) {
		const CODEC_HWParam *param = (const CODEC_HWParam *)arg;
		if (param) {
			mic_type = param->mainmic_type;

			/* ADCs analog PGA gain Config */
			snd_reg_update_bits(codec, ANA_ADC1_CTRL3, 0x1f<<RX1_PGA_GAIN_CTRL, param->mainmic_analog_val<<RX1_PGA_GAIN_CTRL);
			snd_reg_update_bits(codec, ANA_ADC2_CTRL3, 0x1f<<RX2_PGA_GAIN_CTRL, param->headsetmic_val<<RX2_PGA_GAIN_CTRL);

			/* ADC Digital Volume Config */
			snd_reg_update_bits(codec, ADC1_DVOL_CTRL, 0xff, 0xa0);	//param->mainmic_digital_val
			snd_reg_update_bits(codec, ADC2_DVOL_CTRL, 0xff, 0xa0);	//param->mainmic_digital_val
		}
	}

	return HAL_OK;
}

static int32_t AC107_JackDetect(CODEC_Req req, void *arg)
{
	return HAL_OK;
}

static const struct codec_ctl_ops ac107_ctl_ops =  {
    .setRoute       = AC107_SetRoute,
    .setVolume      = AC107_SetVolume,
    .setTrigger     = AC107_SetTrigger,
    .setEqScene     = AC107_SetEqScene,
    .ioctl          = AC107_Ioctl,
//    .regCtl         = AC107_RegCtl,
};

static const struct codec_dai_ops ac107_dai_ops =  {
    .setPll     = AC107_SetPll,
    .setClkdiv  = AC107_SetClkdiv,
    .setFormat  = AC107_SetFormat,
    .shutDown   = AC107_ShutDown,
};

static const struct codec_ops ac107_ops =  {
    .setPower       = AC107_SetPower,
    .setSysClk      = AC107_SetSysClk,
    .setInitParam   = AC107_SetInitParam,
    .jackDetect     = AC107_JackDetect,
//    .attribute      = AC107_Attribute,
};

static HAL_Status AC107_Probe(struct CodecProbeParam *param, struct CodecProbeResult *result)
{
	int i;
	uint8_t val[4];
	int32_t receiveLen;
	uint8_t i2cAddrCnt = 4;
	uint8_t i2cAddr[] = {AC107_I2C_ADDR1, AC107_I2C_ADDR2, AC107_I2C_ADDR3, AC107_I2C_ADDR4};
	uint8_t i2cId = param->i2cId;

	for (i = 0; i < i2cAddrCnt; i++) {
		receiveLen = HAL_I2C_Master_Receive_Mem_IT(i2cId, i2cAddr[i], 0, I2C_MEMADDR_SIZE_8BIT, val, AC107_REG_VALUE_LENGTH);
		if ((receiveLen == AC107_REG_VALUE_LENGTH) && (val[0] == 0x4B)) {
			result->type  = 0;
			result->i2cId = i2cId;
			result->i2cAddr = i2cAddr[i];
			return HAL_OK;
		}
	}
	return HAL_ERROR;
}

static CODEC_INFO *AC107_Create(struct CodecCreateParam *param)
{
	CODEC_INFO *codec_info;
	codec_info = (CODEC_INFO *)malloc(sizeof(CODEC_INFO));
	if (codec_info == NULL) {
		AC107_ERROR("malloc fail.\n");
		return NULL;
	}
	codec_info->ops     = &ac107_ops;
	codec_info->dai_ops = &ac107_dai_ops;
	codec_info->ctl_ops = &ac107_ctl_ops;
	codec_info->i2cId   = param->i2cId;
	codec_info->i2cAddr = param->i2cAddr;
	codec_info->write   = param->write;
	codec_info->read    = param->read;
	codec_info->RegLength    = AC107_REG_LENGTH;
	codec_info->RegValLength = AC107_REG_VALUE_LENGTH;
	codec = codec_info;
	return codec_info;
}

static HAL_Status AC107_Destroy(CODEC_INFO *codec)
{
	free(codec);
	return HAL_OK;
}

const struct CodecCreatorS AC107CodecCtor = {
	.probe   = AC107_Probe,
	.create  = AC107_Create,
	.destroy = AC107_Destroy,
};

