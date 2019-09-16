/**
  * @file  hal_audio_codec.h
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

#if (__CONFIG_CHIP_ARCH_VER == 2)

#ifndef _DRIVER_CHIP_HAL_AUDIOCODEC_H_
#define _DRIVER_CHIP_HAL_AUDIOCODEC_H_

#include "driver/chip/hal_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
  * Audio Codec Controller
  */
typedef struct
{
	__IO uint32_t AC_DAC_DIG_CTRL;   /* CODEC DAC Digital Part Control Register, Address offset: 0x00 */
	__IO uint32_t AC_DAC_FIFO_CTRL;  /* CODEC DAC FIFO Control Register, Address offset: 0x04 */
	__IO uint32_t AC_DAC_FIFO_STA;   /* CODEC DAC FIFO Status Register, Address offset: 0x08 */
	__O  uint32_t AC_DAC_TXDATA;     /* CODEC DAC TX DATA Register,	Address offset: 0x0C */
	__IO uint32_t AC_DAC_TXCNT;      /* CODEC DAC TX FIFO Counter Register, Address offset: 0x10 */
	__IO uint32_t AC_DAC_DEBUG;      /* CODEC DAC Debug Register, Address offset: 0x14 */
	__IO uint32_t AC_ADC_FIFO_CTRL;  /* CODEC ADC FIFO Control Register, Address offset: 0x18 */
	__IO uint32_t AC_ADC_FIFO_STA;   /* CODEC ADC FIFO Status Register, Address offset: 0x1C */
	     uint32_t AC_ADC_RXDATA;     /* CODEC ADC RX Data Register, Address offset: 0x20 */
	__IO uint32_t AC_ADC_RXCNT;      /* CODEC ADC RX Counter Register, ddress offset: 0x24 */
	__IO uint32_t AC_ADC_HPF_CTRL;   /* CODEC ADC HPF Control Register,	Address offset: 0x28 */
	__IO uint32_t AC_ADC_HPF_COEF;   /* CODEC ADC HPF Coef Register,	Address offset: 0x2C */
	__IO uint32_t AC_ADC_HPF_GAIN;   /* CODEC ADC HPF Gain Coef Register, Address offset: 0x30 */
	__IO uint32_t AC_ADDA_DBG_CTRL;  /* CODEC ADDA Debug Control Register, Address offset: 0x34 */
	__IO uint32_t AC_POWER_CTRL;   	 /* CODEC Audio Codec Power Control Register, Address offset: 0x38 */
	__IO uint32_t AC_MBIAS_CTRL;   	 /* CODEC MBIAS Control Register, Address offset: 0x3C */
	__IO uint32_t AC_ADC_ANA_CTRL;   /* CODEC ADC Analog Control Register, Address offset: 0x40 */
	__IO uint32_t AC_DAC_ANA_CTRL;   /* CODEC DAC Analog Control Register, Address offset: 0x44 */
} CODEC_T;

#define CODEC	((CODEC_T *)CODEC_BASE)

/*
 * Bits definition for DAC Digital Part Control Register (0x00)
 */
#define CODEC_DAC_DIG_EN_BIT				HAL_BIT(31)

#define CODEC_DAC_DIG_MODQU_SHIFT			(25)
#define CODEC_DAC_DIG_MODQU_MASK			(0x0FU << CODEC_DAC_DIG_MODQU_SHIFT)	/* Internal DAC Quantization Levelse [28:25] */

#define CODEC_DAC_DIG_HPF_EN_SHIFT			(18)
#define CODEC_DAC_DIG_HPF_EN_MASK			(0x01U << CODEC_DAC_DIG_HPF_EN_SHIFT)	/* High Pass Filter Enable [18] */

#define CODEC_DAC_DIG_VOL_SHIFT				(12)
#define CODEC_DAC_DIG_VOL_MASK				(0x3FU << CODEC_DAC_DIG_VOL_SHIFT)	/* Digital volume control [17:12] */

#define CODEC_DAC_DIG_DAC_MIX_SHIFT			(2)
#define CODEC_DAC_DIG_DAC_MIX_MASK			(0x03U << CODEC_DAC_DIG_DAC_MIX_SHIFT)	/* DAC_Mixer control [3:2] */

#define CODEC_DAC_DIG_TX_MIX_SHIFT			(0)
#define CODEC_DAC_DIG_TX_MIX_MASK			(0x03U << CODEC_DAC_DIG_TX_MIX_SHIFT)	/* TX_Mixer control [1:0] */

/*
 * Bits definition for DAC FIFO Control Register (0x04)
 */
#define CODEC_DAC_FS_SHIFT					(29)
#define CODEC_DAC_FS_MASK					(0x07U << CODEC_DAC_FS_SHIFT)	/* DAC SampleRate [31:29] */

#define CODEC_DAC_FIR_VER_SHIFT				(28)
#define CODEC_DAC_FIR_VER_MASK				(0x01U << CODEC_DAC_FIR_VER_SHIFT)	/* DAC FIR Version [28] */

#define CODEC_DAC_SEND_LASAT_SHIFT			(26)
#define CODEC_DAC_SEND_LASAT_MASK			(0x01U << CODEC_DAC_SEND_LASAT_SHIFT)	/* Audio sample select when TX FIFO under run [26] */

#define CODEC_DAC_FIFO_MODE_SHIFT			(24)
#define CODEC_DAC_FIFO_MODE_MASK			(0x01U << CODEC_DAC_FIFO_MODE_SHIFT)	/* Set transmitted audio sample FIFO mode [24] */

#define CODEC_DAC_DRQ_CLR_CNT_SHIFT			(21)
#define CODEC_DAC_DRQ_CLR_CNT_MASK			(0x03U << CODEC_DAC_DRQ_CLR_CNT_SHIFT)	/* RQ_CLR_CNT [22:21] */

#define CODEC_DAC_TX_TRIG_LEVEL_SHIFT		(8)
#define CODEC_DAC_TX_TRIG_LEVEL_MASK		(0x7FU << CODEC_DAC_TX_TRIG_LEVEL_SHIFT)	/* TX FIFO Empty Trigger Level  [14:8] */

#define CODEC_DAC_MONO_EN_SHIFT				(6)
#define CODEC_DAC_MONO_EN_MASK				(0x1U << CODEC_DAC_MONO_EN_SHIFT)	/* DAC Mono Enable [6] */

#define CODEC_TX_SAMPLE_BITS_SHIFT			(5)
#define CODEC_TX_SAMPLE_BITS_MASK			(0x1U << CODEC_TX_SAMPLE_BITS_SHIFT)	/* Transmitting Audio Sample Resolution [5] */

#define CODEC_TXFIFO_DRQ_EN_BIT				HAL_BIT(4)
#define CODEC_TXFIFO_IRQ_EN_BIT				HAL_BIT(3)
#define CODEC_TXFIFO_URN_IRQ_EN_BIT			HAL_BIT(2)
#define CODEC_TXFIFO_ORN_IRQ_EN_BIT			HAL_BIT(1)
#define CODEC_TXFIFO_FLUSH_BIT              HAL_BIT(0)

/*
 * Bits definition for DAC Debug Register (0x14)
 */
#define CODEC_DAC_MODU_SELECT_SHIFT			(11)
#define CODEC_DAC_MODU_SELECT_MASK			(0x01U << CODEC_DAC_MODU_SELECT_SHIFT)	/* DAC Modulator Debug [11] */

#define CODEC_DAC_PATTERN_SELECT_SHIFT		(9)
#define CODEC_DAC_PATTERN_SELECTT_MASK		(0x03U << CODEC_DAC_PATTERN_SELECT_SHIFT)	/* DAC Pattern Select [10:9] */

#define CODEC_DAC_CLK_SELECT_SHIFT			(8)
#define CODEC_DAC_CLK_SELECT_MASK			(0x1U << CODEC_DAC_CLK_SELECT_SHIFT)	/* CODEC Clock Source Select [8] */

#define CODEC_ADDA_LOOP_MODE_SHIFT			(0)
#define CODEC_ADDA_LOOP_MODE_MASK			(0x1U << CODEC_ADDA_LOOP_MODE_SHIFT)	/* ADDA Loop MODE SELECT [0] */

/*
 * Bits definition for ADC FIFO Control Register (0x18)
 */
#define CODEC_ADC_FS_SHIFT					(29)
#define CODEC_ADC_FS_MASK					(0x07U << CODEC_ADC_FS_SHIFT)	/* ADC SampleRate [31:29] */

#define CODEC_ADC_DIG_EN_BIT				HAL_BIT(28)		/* ADC Digital Part Enable [28] */
#define CODEC_DMIC_DIG_EN_BIT				HAL_BIT(27)		/* DMIC Digital Part Enable [27] */

#define CODEC_ADCFDT_SHIFT					(25)
#define CODEC_ADCFDT_MASK					(0x03U << CODEC_ADCFDT_SHIFT)	/* ADC FIFO Delay Time For writing Data after ADC_DIG_EN [26:25] */

#define CODEC_ADC_DFEN_SHIFT				(24)
#define CODEC_ADC_DFEN_MASK					(0x01U << CODEC_ADC_DFEN_SHIFT)		/* ADC FIFO Delay Function For writing Data after EN_AD [24] */

#define CODEC_RX_FIFO_MODE_SHIFT			(23)
#define CODEC_RX_FIFO_MODE_MASK				(0x01U << CODEC_RX_FIFO_MODE_SHIFT)		/* RX FIFO Output Mode  [23] */

#define CODEC_ADC_DMIC_EN_SHIFT				(22)
#define CODEC_ADC_DMIC_MONO_EN_SHIFT        (21)
#define CODEC_ADC_DMIC_EN_BIT				HAL_BIT(22)		/* ADC DMIC Enable [22] */
#define CODEC_ADC_DMIC_MONO_EN				HAL_BIT(21)		/* ADC DMIC Mono Mode Enable [21] */

#define CODEC_RX_SAMPLE_BITS_SHIFT			(17)
#define CODEC_RX_SAMPLE_BITS_MASK			(0x1U << CODEC_RX_SAMPLE_BITS_SHIFT)	/* Receiving Audio Sample Resolution [17] */

#define CODEC_A2D_CHSEL_SHIFT				(16)
#define CODEC_A2D_CHSEL_MASK				(0x1U << CODEC_A2D_CHSEL_SHIFT)	/* ADC channel Select [16] */

#define CODEC_AD_SWP_SHIFT					(15)
#define CODEC_AD_SWP_MASK					(0x1U << CODEC_AD_SWP_SHIFT)	/* ADC output channel swap enable (for digital filter) [15] */

#define CODEC_DMICR_EN_SHIFT				(14)
#define CODEC_DMICR_EN_MASK					(0x1U << CODEC_DMICR_EN_SHIFT)	/* DMIC Right channel enable [14] */

#define CODEC_DMICL_EN_SHIFT				(13)
#define CODEC_DMICL_EN_MASK					(0x1U << CODEC_DMICL_EN_SHIFT)	/* DMIC Left channel enable [13] */

#define CODEC_ADCR_EN_SHIFT					(12)
#define CODEC_ADCR_EN_MASK					(0x1U << CODEC_ADCR_EN_SHIFT)	/* ADC Right channel enable [12] */

#define CODEC_ADCL_EN_SHIFT					(11)
#define CODEC_ADCL_EN_MASK					(0x1U << CODEC_ADCL_EN_SHIFT)	/* ADCL Left channel enable [11] */

#define CODEC_RX_FIFO_TRG_LEVEL_SHIFT		(4)
#define CODEC_RX_FIFO_TRG_LEVEL_MASK		(0x7FU << CODEC_RX_FIFO_TRG_LEVEL_SHIFT)	/* RX FIFO Trigger Level [10:4] */

#define CODEC_RXFIFO_DRQ_EN_BIT				HAL_BIT(3)
#define CODEC_RXFIFO_IRQ_EN_BIT				HAL_BIT(2)
#define CODEC_RXFIFO_ORN_IRQ_EN_BIT			HAL_BIT(1)
#define CODEC_RXFIFO_FLUSH_BIT              HAL_BIT(0)

/*
 * Bits definition for ADC HPF Control Register (0x28)
 */
#define CODEC_DMIC_HPF_EN_BIT				HAL_BIT(1)
#define CODEC_ADC_HPF_EN_BIT				HAL_BIT(0)

/*
 * Bits definition for ADDA Debug Control Register (0x34)
 */
#define CODEC_ADC_PTN_SEL_SHIFT				(4)
#define CODEC_ADC_PTN_SEL_MASK				(0x01U << CODEC_ADC_PTN_SEL_SHIFT)	/* ADC Pattern Select [4] */

/*
 * Bits definition for Power Control Register (0x38)
 */
#define CODEC_RAMP_TIME_SHIFT				(20)
#define CODEC_RAMP_TIME_MASK				(0x7U << CODEC_RAMP_TIME_SHIFT)	/* Ramp Time [22:20] */

#define CODEC_BG_SPEEDUP_STA_BIT			HAL_BIT(19)
#define CODEC_ADLDO_EN_BIT					HAL_BIT(18)

#define CODEC_ADLDO_VCTRL_SHIFT				(15)
#define CODEC_ADLDO_VCTRL_MASK				(0x7U << CODEC_ADLDO_VCTRL_SHIFT) /* ADLDO Output Voltage Control [17:15] */

#define CODEC_ADLDO_BYPASS_EN_BIT			HAL_BIT(14)
#define CODEC_VRA1_EN_BIT					HAL_BIT(13)

#define CODEC_VRA1_VCTRL_SHIFT				(11)
#define CODEC_VRA1_VCTRL_MASK				(0x3U << CODEC_VRA1_VCTRL_SHIFT)  /* VRA1 Output Voltage Control [12:11] */

#define CODEC_BG_TRIM_SHIFT					(7)
#define CODEC_BG_TRIM_MASK					(0xFU << CODEC_BG_TRIM_SHIFT)

/*
 * Bits definition for ADC Analog Control Register (0x40)
 */
#define CODEC_LINEIN_GAIN_SHIFT				(26)
#define CODEC_LINEIN_GAIN_MASK				(0x7U << CODEC_LINEIN_GAIN_SHIFT)

#define CODEC_LINEIN_OPEN_MASK				HAL_BIT(25)
#define CODEC_ADCR_ANA_EN_MASK				HAL_BIT(24)

#define CODEC_MIC_PGA_GAIN_SHIFT			(10)
#define CODEC_MIC_PGA_GAIN_MASK				(0x7U << CODEC_MIC_PGA_GAIN_SHIFT)

#define CODEC_MIC_PGA_EN_MASK				HAL_BIT(9)
#define CODEC_ADCL_ANA_EN_MASK				HAL_BIT(8)


/*
 * Bits definition for DAC Analog Control Register (0x44)
 */
#define CODEC_DAC_ANA_RAMP_MAN_EN			HAL_BIT(17)	/* RAMP_MAN_EN [17] */
#define CODEC_DAC_PLAY_ANA_EN				HAL_BIT(16)	/* Playback Path Analog Part enable(DAC Analog+Lineout+Antipop) [16] */
#define CODEC_DAC_ANA_EN					HAL_BIT(15)	/* DAC Analog enable [15] */
#define CODEC_ADDA_BIAS_EN					HAL_BIT(9)	/* ADDA BIAS enable [9] */
#define CODEC_RAMP_EN						HAL_BIT(8)	/* RAMP nable [8] */
#define CODEC_DAC_LOUT_EN_SHIFT				(7)			/* Lineout enable [7] */
#define CODEC_DAC_LOUT_EN					HAL_BIT(7)	/* Lineout enable [7] */
#define CODEC_DAC_LOUT_DIFF_EN				HAL_BIT(6)	/* Lineout Differential Mode Enable [6] */
#define CODEC_DAC_LOUT_MUTE					HAL_BIT(5)	/* Lineout Mute control [5] */

#define CODEC_DAC_LOUT_GAIN_SHIFT			(0)
#define CODEC_DAC_LOUT_GAIN_MASK			(0x1FU << CODEC_DAC_LOUT_GAIN_SHIFT)	/* Lineout OP Gain control [4:0] */

/**
  * @brief codec sample rate definition
  */
typedef enum {
	CODEC_SR48_44K    = 0,  /* 48000 or 44100Hz  */
	CODEC_SR32K       = 1,  /* 32000Hz  */
	CODEC_SR24_22K    = 2,  /* 24000 or 22050Hz  */
	CODEC_SR16K       = 3,  /* 16000Hz  */
	CODEC_SR12_11K    = 4,  /* 12000 or 11025Hz  */
	CODEC_SR8K        = 5,  /* 8000Hz   */
	CODEC_SR192K      = 6,  /* 192000Hz */
	CODEC_SR96K       = 7,  /* 96000Hz  */
} CODEC_SampleRate;

/**
  * @brief stream direction
  */
typedef enum {
	CODEC_PLAYBACK,
	CODEC_RECORD,
} CODEC_StreamDir;

/**
  * @brief sampling accuracy
  */
typedef enum {
	CODEC_SR16BIT = 0,
	CODEC_SR24BIT = 1,
} CODEC_Resolution;

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_CHIP_HAL_AUDIOCODEC_H_ */
#endif /* (__CONFIG_CHIP_ARCH_VER == 2) */
