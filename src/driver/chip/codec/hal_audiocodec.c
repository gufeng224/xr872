/**
  * @file  hal_audiocodec.c
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

#include "../hal_base.h"
#include "driver/chip/codec.h"
#include "hal_audiocodec.h"
#include "driver/chip/hal_dma.h"

#define CODEC_DBG_ON              1
#define CODEC_WRN_ON              1
#define CODEC_ERR_ON              1

#if (CODEC_DBG_ON == 1)
#define CODEC_DBG(fmt, arg...)    HAL_LOG(CODEC_DBG_ON, "[AUDIO_CODEC] "fmt, ##arg)
#else
#define CODEC_DBG(fmt, arg...)
#endif

#if (CODEC_WRN_ON == 1)
#define CODEC_WRN(fmt, arg...)    HAL_LOG(CODEC_WRN_ON, "[AUDIO_CODEC] "fmt, ##arg)
#else
#define CODEC_WRN(fmt, arg...)
#endif

#if (CODEC_ERR_ON == 1)
#define CODEC_ERR(fmt, arg...)    HAL_LOG(CODEC_ERR_ON, "[AUDIO_CODEC] "fmt, ##arg)
#else
#define CODEC_ERR(fmt, arg...)
#endif

#define CODEC_MEMCPY              HAL_Memcpy
#define CODEC_MALLOC              HAL_Malloc
#define CODEC_FREE                HAL_Free
#define CODEC_MEMSET              HAL_Memset

#define CODEC_REG_DBG_ON          0
#define UNDERRUN_THRESHOLD        3
#define OVERRUN_THRESHOLD         3

typedef struct {
	volatile bool				isHwInit;
	volatile bool				txRunning;
	volatile bool				rxRunning;

	uint8_t 					*txBuf;
	uint8_t 					*rxBuf;
	uint8_t 					*readPointer;
	uint8_t 					*writePointer;
	uint32_t					rxBufSize;
	uint32_t					txBufSize;

	DMA_Channel 				txDMAChan;
	DMA_Channel 				rxDMAChan;
	volatile uint8_t		   txHalfCallCount;
	volatile uint8_t		   rxHalfCallCount;
	volatile uint8_t		   txEndCallCount;
	volatile uint8_t		   rxEndCallCount;
	uint8_t 					*txDmaPointer;
	uint8_t 					*rxDmaPointer;

	HAL_Semaphore				txReady;
	HAL_Semaphore				rxReady;
	bool						isTxSemaphore;
	bool						isRxSemaphore;
	bool						isTxInitiate;
	bool						isRxInitiate;

	uint32_t                    audioPllParam;
    uint32_t                    audioPllPatParam;
} AUDIO_CODEC_Priv;

static AUDIO_CODEC_Priv gAudioCodecPriv;

typedef enum {
        AUD_PLL_24M        = 0U,
        AUD_PLL_22M        = 1U,
} AUDIO_PLLMode;

typedef struct {
        uint32_t        hosc;
        uint32_t        audio;
        uint32_t        pllParam;
        uint32_t        pllPatParam;
} CODEC_HOSC_Type;

static const CODEC_HOSC_Type codec_hosc_aud_type[] = {
        {HOSC_CLOCK_26M, AUD_PLL_24M, PRCM_AUD_PLL24M_PARAM_HOSC26M, PRCM_AUD_PLL24M_PAT_PARAM_HOSC26M},
        {HOSC_CLOCK_26M, AUD_PLL_22M, PRCM_AUD_PLL22M_PARAM_HOSC26M, PRCM_AUD_PLL22M_PAT_PARAM_HOSC26M},
        {HOSC_CLOCK_24M, AUD_PLL_24M, PRCM_AUD_PLL24M_PARAM_HOSC24M, PRCM_AUD_PLL24M_PAT_PARAM_HOSC24M},
        {HOSC_CLOCK_24M, AUD_PLL_22M, PRCM_AUD_PLL22M_PARAM_HOSC24M, PRCM_AUD_PLL22M_PAT_PARAM_HOSC24M},
        {HOSC_CLOCK_40M, AUD_PLL_24M, PRCM_AUD_PLL24M_PARAM_HOSC40M, PRCM_AUD_PLL24M_PAT_PARAM_HOSC40M},
        {HOSC_CLOCK_40M, AUD_PLL_22M, PRCM_AUD_PLL22M_PARAM_HOSC40M, PRCM_AUD_PLL22M_PAT_PARAM_HOSC40M},
        {HOSC_CLOCK_52M, AUD_PLL_24M, PRCM_AUD_PLL24M_PARAM_HOSC52M, PRCM_AUD_PLL24M_PAT_PARAM_HOSC52M},
        {HOSC_CLOCK_52M, AUD_PLL_22M, PRCM_AUD_PLL22M_PARAM_HOSC52M, PRCM_AUD_PLL22M_PAT_PARAM_HOSC52M},
};

static void CODEC_PowerOn(void)
{
	HAL_SET_BIT(CODEC->AC_POWER_CTRL, CODEC_VRA1_EN_BIT | CODEC_ADLDO_EN_BIT);
	HAL_SET_BIT(CODEC->AC_DAC_ANA_CTRL, CODEC_ADDA_BIAS_EN);
}

static void CODEC_PowerOff(void)
{
	HAL_CLR_BIT(CODEC->AC_POWER_CTRL, CODEC_VRA1_EN_BIT | CODEC_ADLDO_EN_BIT);
	HAL_CLR_BIT(CODEC->AC_DAC_ANA_CTRL, CODEC_ADDA_BIAS_EN);
}

static void CODEC_DisableTx()
{
	HAL_CLR_BIT(CODEC->AC_DAC_DIG_CTRL, CODEC_DAC_DIG_EN_BIT);
}

static void CODEC_EnableTx()
{
	HAL_SET_BIT(CODEC->AC_DAC_DIG_CTRL, CODEC_DAC_DIG_EN_BIT);
}

static void CODEC_DisableRx()
{
	HAL_CLR_BIT(CODEC->AC_ADC_FIFO_CTRL, CODEC_ADC_DIG_EN_BIT);
}

static void CODEC_EnableRx()
{
	HAL_SET_BIT(CODEC->AC_ADC_FIFO_CTRL, CODEC_ADC_DIG_EN_BIT);
}

uint32_t CODEC_AUDIOPLL_Update(AUDIO_PLLMode pll)
{
    AUDIO_CODEC_Priv *priv = &gAudioCodecPriv;

    if (pll != AUD_PLL_24M &&  pll != AUD_PLL_22M)
            return -1;

    uint32_t hoscClock = HAL_GetHFClock();

    int i = 0;
    for (i = 0; i < HAL_ARRAY_SIZE(codec_hosc_aud_type); i++) {
            if ((codec_hosc_aud_type[i].hosc == hoscClock) && (codec_hosc_aud_type[i].audio == pll)) {
                    priv->audioPllParam = codec_hosc_aud_type[i].pllParam;
                    priv->audioPllPatParam = codec_hosc_aud_type[i].pllPatParam;
                    break;
            }
    }
    if (i == HAL_ARRAY_SIZE(codec_hosc_aud_type)) {
            CODEC_ERR("Update audio pll failed....\n");
            return -1;
    }
    return 0;
}

static HAL_Status CODEC_SET_InitCfg(CODEC_DataParam *param)
{
    if (!param)
		return -1;

    if (param->direction == CODEC_PLAYBACK) {/*play*/

		uint32_t dac_dig_reg_mask = CODEC_DAC_DIG_MODQU_MASK | CODEC_DAC_DIG_HPF_EN_MASK |
									CODEC_DAC_DIG_VOL_MASK | CODEC_DAC_DIG_DAC_MIX_MASK;
		uint32_t dac_dig_reg_val = (0x0 << CODEC_DAC_DIG_MODQU_SHIFT) | (0x0 << CODEC_DAC_DIG_HPF_EN_SHIFT) |
								   (0x0 << CODEC_DAC_DIG_VOL_SHIFT) | (0x0 << CODEC_DAC_DIG_DAC_MIX_SHIFT);
		HAL_MODIFY_REG(CODEC->AC_DAC_DIG_CTRL, dac_dig_reg_mask, dac_dig_reg_val);

		uint32_t dac_fifo_reg_mask = CODEC_DAC_FIR_VER_MASK | CODEC_DAC_SEND_LASAT_MASK |
									 CODEC_DAC_FIFO_MODE_MASK | CODEC_DAC_DRQ_CLR_CNT_MASK |
									 CODEC_DAC_TX_TRIG_LEVEL_MASK | CODEC_DAC_MONO_EN_MASK;
		uint32_t dac_fifo_reg_val = (0x0 << CODEC_DAC_FIR_VER_SHIFT) | (0x0 << CODEC_DAC_SEND_LASAT_SHIFT) |
									(0x0 << CODEC_DAC_FIFO_MODE_SHIFT) | (0x0 << CODEC_DAC_DRQ_CLR_CNT_SHIFT) |
									(0x40 << CODEC_DAC_TX_TRIG_LEVEL_SHIFT) | (0x0 << CODEC_DAC_MONO_EN_SHIFT);
		HAL_MODIFY_REG(CODEC->AC_DAC_FIFO_CTRL, dac_fifo_reg_mask, dac_fifo_reg_val);


		uint32_t adc_dbg_reg_mask = CODEC_DAC_MODU_SELECT_MASK | CODEC_DAC_PATTERN_SELECTT_MASK |
									CODEC_DAC_CLK_SELECT_MASK | CODEC_ADDA_LOOP_MODE_MASK;
		uint32_t adc_dbg_reg_val = (0x0 <<CODEC_DAC_MODU_SELECT_SHIFT) | (0x0 <<CODEC_DAC_PATTERN_SELECT_SHIFT) |
								   (0x0 <<CODEC_DAC_CLK_SELECT_SHIFT) | (0x0 <<CODEC_ADDA_LOOP_MODE_SHIFT);
		HAL_MODIFY_REG(CODEC->AC_DAC_DEBUG, adc_dbg_reg_mask, adc_dbg_reg_val);

		HAL_CLR_BIT(CODEC->AC_DAC_ANA_CTRL, CODEC_DAC_LOUT_DIFF_EN);
    } else {/*record*/
		uint32_t adc_fifo_reg_mask = CODEC_ADCFDT_MASK | CODEC_ADC_DFEN_MASK |
									 CODEC_RX_FIFO_MODE_MASK |
									 CODEC_A2D_CHSEL_MASK | CODEC_AD_SWP_MASK |
									 CODEC_RX_FIFO_TRG_LEVEL_MASK;
		uint32_t adc_fifo_reg_val = (0x0 << CODEC_ADCFDT_SHIFT) | (0x0 << CODEC_ADC_DFEN_SHIFT) |
								    (0x0 << CODEC_RX_FIFO_MODE_SHIFT) |
								    (0x0 << CODEC_A2D_CHSEL_SHIFT) | (0x0 << CODEC_AD_SWP_SHIFT) |
								    (0x40 << CODEC_RX_FIFO_TRG_LEVEL_SHIFT);
        HAL_MODIFY_REG(CODEC->AC_ADC_FIFO_CTRL, adc_fifo_reg_mask, adc_fifo_reg_val);

		HAL_MODIFY_REG(CODEC->AC_ADDA_DBG_CTRL, CODEC_ADC_PTN_SEL_MASK, (0x0 << CODEC_ADC_PTN_SEL_SHIFT));

		HAL_SET_BIT(CODEC->AC_ADC_HPF_CTRL, CODEC_ADC_HPF_EN_BIT | CODEC_DMIC_HPF_EN_BIT);
    }
    return HAL_OK;
}

static HAL_Status CODEC_SET_ClkDiv(CODEC_DataParam *param)
{
    if (!param)
		return HAL_INVALID;

	CODEC_SampleRate sr = CODEC_SR48_44K;
	CODEC_StreamDir dir = param->direction;
	CODEC_Resolution res = param->resolution;
	switch (param->sampleRate) {
		case 192000:
			if (dir == CODEC_RECORD)
				return HAL_INVALID;
			sr = CODEC_SR192K;
			break;
		case 96000:
			if (dir == CODEC_RECORD)
				return HAL_INVALID;
			sr = CODEC_SR96K;
			break;
		case 48000:
		case 44100:
			sr = CODEC_SR48_44K;
			break;
		case 8000:
			sr = CODEC_SR8K;
			break;
		case 12000:
		case 11025:
			sr = CODEC_SR12_11K;
			break;
		case 16000:
			sr = CODEC_SR16K;
			break;
		case 24000:
		case 22050:
			sr = CODEC_SR24_22K;
			break;
		case 32000:
			sr = CODEC_SR32K;
			break;
		default:
			return HAL_INVALID;
			break;
	}

	CODEC_DBG("SampleRate: %d...\n", param->sampleRate);

	uint32_t audioPll = AUD_PLL_24M;
	if ((param->sampleRate % 1000) != 0)
         audioPll = AUD_PLL_22M;

	AUDIO_CODEC_Priv *priv = &gAudioCodecPriv;

	/*set sysclk*/
	CODEC_AUDIOPLL_Update(audioPll);
	HAL_PRCM_SetAudioPLLParam(priv->audioPllParam);
	HAL_PRCM_EnableAudioPLL();

	/*set sampleRate、resolution*/
	if (dir == CODEC_PLAYBACK) {
		HAL_MODIFY_REG(CODEC->AC_DAC_FIFO_CTRL, CODEC_DAC_FS_MASK | CODEC_TX_SAMPLE_BITS_MASK,
						(sr << CODEC_DAC_FS_SHIFT) | (res << CODEC_TX_SAMPLE_BITS_SHIFT));
	} else {
		HAL_MODIFY_REG(CODEC->AC_ADC_FIFO_CTRL, CODEC_ADC_FS_MASK | CODEC_RX_SAMPLE_BITS_MASK,
						(sr << CODEC_ADC_FS_SHIFT) | (res << CODEC_RX_SAMPLE_BITS_SHIFT));
	}

	return 0;
}

static HAL_Status CODEC_SET_Channels(CODEC_DataParam *param)
{
    if (!param)
		return -1;

    if (param->direction == CODEC_PLAYBACK) {/*play*/
        if (param->channels <= 0 || param->channels > 2) {
            CODEC_ERR("Invalid channels num,failed (%d)...\n",param->channels);
            return -1;
        }
		uint8_t mono_en = (param->channels == 1) ? 0x01 : 0x0;
        HAL_MODIFY_REG(CODEC->AC_DAC_DIG_CTRL, CODEC_DAC_DIG_TX_MIX_MASK, 0x03 << CODEC_DAC_DIG_TX_MIX_SHIFT);/* 0x0: TXL 0x03: (TXL+TXR)/2 */
		HAL_MODIFY_REG(CODEC->AC_DAC_FIFO_CTRL, CODEC_DAC_MONO_EN_MASK, mono_en << CODEC_DAC_MONO_EN_SHIFT);
    } else {/*record*/
        if (param->channels <= 0 || param->channels > 4){
            CODEC_ERR("Invalid usr rx channels num,failed (%d)...\n",param->channels);
            return -1;
        }
#if 0
		if (param->channels == 1) {
			HAL_MODIFY_REG(CODEC->AC_ADC_FIFO_CTRL, CODEC_ADCR_EN_MASK, 0x01 << CODEC_ADCR_EN_SHIFT);
		} else
			HAL_MODIFY_REG(CODEC->AC_ADC_FIFO_CTRL, CODEC_ADCL_EN_MASK|CODEC_ADCR_EN_MASK,
				0x01 << CODEC_ADCL_EN_SHIFT|0x01 << CODEC_ADCR_EN_SHIFT);
#endif
    }

    return 0;
}

static void CODEC_DMAStart(DMA_Channel chan, uint32_t srcAddr, uint32_t dstAddr, uint32_t datalen)
{
	HAL_DMA_Start(chan, srcAddr, dstAddr, datalen);
}

static void CODEC_DMAStop(DMA_Channel chan)
{
	HAL_DMA_Stop(chan);
}

static void tx_enable(bool enable)
{
    AUDIO_CODEC_Priv *priv = &gAudioCodecPriv;
    /*clear tx tifo*/
    HAL_SET_BIT(CODEC->AC_DAC_FIFO_CTRL, CODEC_TXFIFO_FLUSH_BIT);

    if (enable) {
		if (priv->txDMAChan != DMA_CHANNEL_INVALID)
			HAL_SET_BIT(CODEC->AC_DAC_FIFO_CTRL, CODEC_TXFIFO_DRQ_EN_BIT);
    } else {
		if (priv->txDMAChan != DMA_CHANNEL_INVALID)
			HAL_CLR_BIT(CODEC->AC_DAC_FIFO_CTRL, CODEC_TXFIFO_DRQ_EN_BIT);
    }
}

static void rx_enable(bool enable)
{
	AUDIO_CODEC_Priv *priv = &gAudioCodecPriv;
	/*clear rx tifo*/
	HAL_SET_BIT(CODEC->AC_ADC_FIFO_CTRL, CODEC_RXFIFO_FLUSH_BIT);

	if (enable) {
		if (priv->rxDMAChan != DMA_CHANNEL_INVALID)
			HAL_SET_BIT(CODEC->AC_ADC_FIFO_CTRL, CODEC_RXFIFO_DRQ_EN_BIT);
	} else {

	if (priv->rxDMAChan != DMA_CHANNEL_INVALID)
		HAL_CLR_BIT(CODEC->AC_ADC_FIFO_CTRL, CODEC_RXFIFO_DRQ_EN_BIT);
	}
}

static void CODEC_DMA_Trigger(bool enable, CODEC_StreamDir dir)
{
    unsigned long flags;
    AUDIO_CODEC_Priv *priv = &gAudioCodecPriv;
    int doProtection = !HAL_IsISRContext();

	if (doProtection) {
		flags = HAL_EnterCriticalSection();
	}

	if (enable) {
		if (dir == CODEC_PLAYBACK) {
			/*trigger tx*/
			tx_enable(enable);

			/* start dma*/
			if (priv->txDMAChan != DMA_CHANNEL_INVALID) {
				CODEC_DMAStart(priv->txDMAChan, (uint32_t)priv->txBuf,
			                        (uint32_t)&(CODEC->AC_DAC_TXDATA), priv->txBufSize);
			}
			priv->txRunning = true;
        } else {
			rx_enable(enable);
			if (priv->rxDMAChan != DMA_CHANNEL_INVALID)
				CODEC_DMAStart(priv->rxDMAChan, (uint32_t)&(CODEC->AC_ADC_RXDATA),
				                (uint32_t)priv->rxBuf, priv->rxBufSize);
			priv->rxRunning = true;
		}
	} else {
            if (dir == CODEC_PLAYBACK) {
                    tx_enable(enable);
                    if (priv->txDMAChan != DMA_CHANNEL_INVALID)
                            CODEC_DMAStop(priv->txDMAChan);
                    priv->txRunning = false;
            } else {
                    rx_enable(enable);
                    if (priv->rxDMAChan != DMA_CHANNEL_INVALID)
                            CODEC_DMAStop(priv->rxDMAChan);
                    priv->rxRunning = false;
            }
	}
#if CODEC_REG_DBG_ON
	CODEC_DBG("CODEC->AC_DAC_DIG_CTRL: 0x%x\n", CODEC->AC_DAC_DIG_CTRL);
	CODEC_DBG("CODEC->AC_DAC_FIFO_CTRL: 0x%x\n", CODEC->AC_DAC_FIFO_CTRL);
	CODEC_DBG("CODEC->AC_DAC_FIFO_STA: 0x%x\n", CODEC->AC_DAC_FIFO_STA);
	CODEC_DBG("CODEC->AC_ADC_FIFO_CTRL: 0x%x\n", CODEC->AC_ADC_FIFO_CTRL);
	CODEC_DBG("CODEC->AC_POWER_CTRL: 0x%x\n", CODEC->AC_POWER_CTRL);
	CODEC_DBG("CODEC->AC_MBIAS_CTRL: 0x%x\n", CODEC->AC_MBIAS_CTRL);
	CODEC_DBG("CODEC->AC_ADC_ANA_CTRL: 0x%x\n", CODEC->AC_ADC_ANA_CTRL);
	CODEC_DBG("CODEC->AC_DAC_ANA_CTRL: 0x%x\n", CODEC->AC_DAC_ANA_CTRL);
#endif
	if (doProtection) {
            HAL_ExitCriticalSection(flags);
	}
}

static int CODEC_DMA_BUFFER_CHECK_Threshold(uint8_t dir)
{
	AUDIO_CODEC_Priv *priv = &gAudioCodecPriv;
	if (dir == 0) {
		if (priv->txHalfCallCount >= UNDERRUN_THRESHOLD ||
					priv->txEndCallCount >= UNDERRUN_THRESHOLD) {
			CODEC_ERR("Tx : underrun and stop dma tx...\n");
			CODEC_DMA_Trigger(false,CODEC_PLAYBACK);/*stop*/
			priv->txRunning = false;
			priv->writePointer = NULL;
			priv->txHalfCallCount = 0;
			priv->txEndCallCount = 0;
			priv->txDmaPointer = NULL;
			return -1;
		}
	} else {
		if (priv->rxHalfCallCount >= OVERRUN_THRESHOLD ||
						priv->rxEndCallCount >= OVERRUN_THRESHOLD) {
			CODEC_ERR("Rx : overrun and stop dma rx...\n");
			CODEC_DMA_Trigger(false,CODEC_RECORD);/*stop*/
			priv->rxRunning = false;
			priv->rxHalfCallCount = 0;
			priv->rxEndCallCount = 0;
			priv->readPointer = NULL;
			priv->rxDmaPointer = NULL;
			return -1;
		}
	}
	return 0;
}

static void CODEC_DMAHalfCallback(void *arg)
{
    AUDIO_CODEC_Priv *priv = &gAudioCodecPriv;
    if (arg == &(priv->txReady)) {
        priv->txHalfCallCount ++;
		if (priv->isTxSemaphore) {
			priv->isTxSemaphore = false;
			HAL_SemaphoreRelease((HAL_Semaphore *)arg);
        }
        if (CODEC_DMA_BUFFER_CHECK_Threshold(0) != 0)
			return;
        priv->txDmaPointer = priv->txBuf + priv->txBufSize/2;
    } else {
		priv->rxHalfCallCount ++;
		if (priv->isRxSemaphore) {
			priv->isRxSemaphore = false;
			HAL_SemaphoreRelease((HAL_Semaphore *)arg);
		}
		if (CODEC_DMA_BUFFER_CHECK_Threshold(1) != 0)
			return;
		priv->rxDmaPointer = priv->rxBuf + priv->rxBufSize/2;
    }
}

static void CODEC_DMAEndCallback(void *arg)
{
    AUDIO_CODEC_Priv *priv = &gAudioCodecPriv;
    if (arg == &(priv->txReady)) {
            priv->txEndCallCount ++;
			if (priv->isTxSemaphore) {
                    priv->isTxSemaphore = false;
                    HAL_SemaphoreRelease((HAL_Semaphore *)arg);
            }
            if (CODEC_DMA_BUFFER_CHECK_Threshold(0) != 0)
                    return;
            priv->txDmaPointer = priv->txBuf;
    } else {
            priv->rxEndCallCount ++;
			if (priv->isRxSemaphore) {
				priv->isRxSemaphore = false;
				HAL_SemaphoreRelease((HAL_Semaphore *)arg);
			}
            if (CODEC_DMA_BUFFER_CHECK_Threshold(1) != 0)
                    return;
            priv->rxDmaPointer = priv->rxBuf;
    }
}

static void CODEC_DMASet(DMA_Channel channel,CODEC_StreamDir dir)
{
#if HAL_DMA_OPT_TRANSFER_HALF_IRQ
        AUDIO_CODEC_Priv *priv = &gAudioCodecPriv;
        DMA_ChannelInitParam dmaParam;
        CODEC_MEMSET(&dmaParam, 0, sizeof(dmaParam));
        if (dir == CODEC_PLAYBACK) {

                dmaParam.cfg = HAL_DMA_MakeChannelInitCfg(DMA_WORK_MODE_CIRCULAR,
                                DMA_WAIT_CYCLE_2,
                                DMA_BYTE_CNT_MODE_REMAIN,
                                DMA_DATA_WIDTH_16BIT,
                                DMA_BURST_LEN_1,
                                DMA_ADDR_MODE_FIXED,
                                DMA_PERIPH_AUDIO_CODEC,
                                DMA_DATA_WIDTH_16BIT,
                                DMA_BURST_LEN_1,
                                DMA_ADDR_MODE_INC,
                                DMA_PERIPH_SRAM);

                dmaParam.endArg = &(priv->txReady);
                dmaParam.halfArg = &(priv->txReady);
        } else {

                dmaParam.cfg = HAL_DMA_MakeChannelInitCfg(DMA_WORK_MODE_CIRCULAR,
                                DMA_WAIT_CYCLE_2,
                                DMA_BYTE_CNT_MODE_REMAIN,
                                DMA_DATA_WIDTH_16BIT,
                                DMA_BURST_LEN_1,
                                DMA_ADDR_MODE_INC,
                                DMA_PERIPH_SRAM,
                                DMA_DATA_WIDTH_16BIT,
                                DMA_BURST_LEN_1,
                                DMA_ADDR_MODE_FIXED,
                                DMA_PERIPH_AUDIO_CODEC);

                dmaParam.endArg = &(priv->rxReady);
                dmaParam.halfArg = &(priv->rxReady);
        }
        dmaParam.irqType = DMA_IRQ_TYPE_BOTH;
        dmaParam.endCallback = CODEC_DMAEndCallback;
        dmaParam.halfCallback = CODEC_DMAHalfCallback;
        HAL_DMA_Init(channel, &dmaParam);
#endif
}

static void CODEC_SetMainMic()
{
	CODEC_DBG("Route(cap): main mic..\n");
	HAL_MODIFY_REG(CODEC->AC_ADC_ANA_CTRL, CODEC_MIC_PGA_GAIN_MASK|CODEC_MIC_PGA_EN_MASK|CODEC_ADCL_ANA_EN_MASK,
						(0x06 << CODEC_MIC_PGA_GAIN_SHIFT)|CODEC_MIC_PGA_EN_MASK|CODEC_ADCL_ANA_EN_MASK);

	HAL_MODIFY_REG(CODEC->AC_ADC_FIFO_CTRL, CODEC_ADCL_EN_MASK, 0x01 << CODEC_ADCL_EN_SHIFT);
}

static void CODEC_SetLineIn()
{
	CODEC_DBG("Route(cap): linein..\n");
	HAL_MODIFY_REG(CODEC->AC_ADC_ANA_CTRL, CODEC_LINEIN_GAIN_MASK|CODEC_LINEIN_OPEN_MASK|CODEC_ADCR_ANA_EN_MASK,
						(0x06 << CODEC_LINEIN_GAIN_SHIFT)|CODEC_LINEIN_OPEN_MASK|CODEC_ADCR_ANA_EN_MASK);

	HAL_MODIFY_REG(CODEC->AC_ADC_FIFO_CTRL, CODEC_ADCR_EN_MASK, 0x01 << CODEC_ADCR_EN_SHIFT);
}

static void CODEC_SetDigitalMic()
{
	CODEC_DBG("Route(cap): digital mic..\n");

	HAL_MODIFY_REG(CODEC->AC_ADC_FIFO_CTRL, CODEC_DMICL_EN_MASK|CODEC_DMICR_EN_MASK|CODEC_DMIC_DIG_EN_BIT,
						(0x01 << CODEC_DMICL_EN_SHIFT)|(0x01 << CODEC_DMICR_EN_SHIFT)|CODEC_DMIC_DIG_EN_BIT);
}

static void CODEC_SetSpeaker()
{
	CODEC_DBG("Route(play): speaker..\n");
	HAL_SET_BIT(CODEC->AC_DAC_ANA_CTRL, CODEC_DAC_PLAY_ANA_EN);
}

static int32_t CODEC_SetRouteDisable(AUDIO_Device device)
{
	switch(device) {
		case AUDIO_IN_DEV_MAINMIC:
			HAL_MODIFY_REG(CODEC->AC_ADC_FIFO_CTRL, CODEC_ADCL_EN_MASK, 0x0<<CODEC_ADCL_EN_SHIFT);
			HAL_CLR_BIT(CODEC->AC_ADC_ANA_CTRL, CODEC_MIC_PGA_EN_MASK|CODEC_ADCL_ANA_EN_MASK);
			break;
		case AUDIO_IN_DEV_LINEIN:
			HAL_MODIFY_REG(CODEC->AC_ADC_FIFO_CTRL, CODEC_ADCR_EN_MASK, 0x0<<CODEC_ADCR_EN_SHIFT);
			HAL_CLR_BIT(CODEC->AC_ADC_ANA_CTRL, CODEC_LINEIN_OPEN_MASK|CODEC_ADCR_ANA_EN_MASK);
			break;
		case AUDIO_IN_DEV_DIGITALMIC:
			HAL_MODIFY_REG(CODEC->AC_ADC_FIFO_CTRL, CODEC_DMICL_EN_MASK|CODEC_DMICR_EN_MASK|CODEC_DMIC_DIG_EN_BIT,
								(0x0<<CODEC_DMICL_EN_SHIFT)|(0x0<<CODEC_DMICR_EN_SHIFT));
			break;
		case AUDIO_OUT_DEV_SPEAKER:
			HAL_CLR_BIT(CODEC->AC_DAC_ANA_CTRL, CODEC_DAC_PLAY_ANA_EN);
			break;
		default:
			CODEC_DBG("invalid dev..\n");
			return -1;
		}

	return 0;
}

static int32_t CODEC_SetRoute(AUDIO_Device device, CODEC_DevState state)
{
	if (state == CODEC_DEV_DISABLE) {
		CODEC_SetRouteDisable(device);
	} else {
		switch(device) {
			case AUDIO_IN_DEV_MAINMIC:
				CODEC_SetMainMic();
				break;
			case AUDIO_IN_DEV_LINEIN:
				CODEC_SetLineIn();
				break;
			case AUDIO_IN_DEV_DIGITALMIC:
				CODEC_SetDigitalMic();
				break;
			case AUDIO_OUT_DEV_SPEAKER:
				CODEC_SetSpeaker();
				break;
			default:
				CODEC_DBG("invalid dev..\n");
				return -1;
		}
	}

	return 0;
}

static int32_t CODEC_SetVolume(AUDIO_Device dev, uint8_t volume)
{
	CODEC_DBG("[set volume] dev(%d) volume(%d)..\n", (int)dev, (int)volume);

	switch (dev) {
		case AUDIO_IN_DEV_MAINMIC:
			if (volume > VOLUME_LEVEL7)
				return -1;
			HAL_MODIFY_REG(CODEC->AC_ADC_ANA_CTRL, 0x7U<<CODEC_MIC_PGA_GAIN_SHIFT, (uint32_t)volume<<CODEC_MIC_PGA_GAIN_SHIFT);
			break;
		case AUDIO_IN_DEV_LINEIN:
			if (volume > VOLUME_LEVEL7)
				return -1;
			HAL_MODIFY_REG(CODEC->AC_ADC_ANA_CTRL, 0x7U<<CODEC_LINEIN_GAIN_SHIFT, (uint32_t)volume<<CODEC_LINEIN_GAIN_SHIFT);
			break;
		case AUDIO_IN_DEV_DIGITALMIC:
			CODEC_DBG("[set volume] DMIC don't support ..\n");
			break;
		case AUDIO_OUT_DEV_SPEAKER:
			if (volume > VOLUME_MAX_LEVEL)
				return -1;
			HAL_MODIFY_REG(CODEC->AC_DAC_ANA_CTRL, CODEC_DAC_LOUT_GAIN_MASK, (uint32_t)volume<<CODEC_DAC_LOUT_GAIN_SHIFT);
			break;
		default:
			CODEC_DBG("[set volume] Wrong audio device..\n");
			return -1;
	}

	return 0;
}

static int32_t CODEC_SetAdda_Direct(AUDIO_Device device, CODEC_DevState state)
{
	if (state == CODEC_DEV_ENABLE) {
		//DAC Mixer Select ADC_DAT
		HAL_MODIFY_REG(CODEC->AC_DAC_DIG_CTRL, CODEC_DAC_DIG_DAC_MIX_MASK, 0x1<<CODEC_DAC_DIG_DAC_MIX_SHIFT);
		CODEC_SetRoute(AUDIO_OUT_DEV_SPEAKER, 1);

		switch(device) {
			case AUDIO_IN_DEV_MAINMIC:
				CODEC_SetMainMic();
				HAL_MODIFY_REG(CODEC->AC_ADC_FIFO_CTRL, 0x1U<<CODEC_ADC_DMIC_EN_SHIFT, 0x0U<<CODEC_ADC_DMIC_EN_SHIFT);	//ADC_DMIC MUX Select ADC
				HAL_MODIFY_REG(CODEC->AC_ADC_FIFO_CTRL, 0x1U<<CODEC_A2D_CHSEL_SHIFT, 0x0U<<CODEC_A2D_CHSEL_SHIFT);		//ADC_TO_DAC MUX Select ADC Left Channel
				break;
			case AUDIO_IN_DEV_LINEIN:
				CODEC_SetLineIn();
				HAL_MODIFY_REG(CODEC->AC_ADC_FIFO_CTRL, 0x1U<<CODEC_ADC_DMIC_EN_SHIFT, 0x0U<<CODEC_ADC_DMIC_EN_SHIFT);	//ADC_DMIC MUX Select ADC
				HAL_MODIFY_REG(CODEC->AC_ADC_FIFO_CTRL, 0x1U<<CODEC_A2D_CHSEL_SHIFT, 0x1U<<CODEC_A2D_CHSEL_SHIFT);		//ADC_TO_DAC MUX Select ADC Right Channel
				break;
			case AUDIO_IN_DEV_DIGITALMIC:
				CODEC_SetDigitalMic();
				HAL_MODIFY_REG(CODEC->AC_ADC_FIFO_CTRL, 0x1U<<CODEC_ADC_DMIC_EN_SHIFT, 0x1U<<CODEC_ADC_DMIC_EN_SHIFT);	//ADC_DMIC MUX Select DMIC
				HAL_MODIFY_REG(CODEC->AC_ADC_FIFO_CTRL, 0x1U<<CODEC_A2D_CHSEL_SHIFT, 0x0U<<CODEC_A2D_CHSEL_SHIFT);		//ADC_TO_DAC MUX Select DMIC Left Channel
				break;
			default:
				CODEC_DBG("invalid dev..\n");
				return -1;
		}
	} else if (state == CODEC_DEV_DISABLE) {
		//DAC Mixer && ADC_DMIC MUX && ADC_TO_DAC MUX recover to default status
		HAL_MODIFY_REG(CODEC->AC_DAC_DIG_CTRL, CODEC_DAC_DIG_DAC_MIX_MASK, 0x0<<CODEC_DAC_DIG_DAC_MIX_SHIFT);	//DAC Mixer Select TX_MIX_OUT
		HAL_MODIFY_REG(CODEC->AC_ADC_FIFO_CTRL, 0x1U<<CODEC_ADC_DMIC_EN_SHIFT, 0x0U<<CODEC_ADC_DMIC_EN_SHIFT);	//ADC_DMIC MUX Select ADC
		HAL_MODIFY_REG(CODEC->AC_ADC_FIFO_CTRL, 0x1U<<CODEC_A2D_CHSEL_SHIFT, 0x0U<<CODEC_A2D_CHSEL_SHIFT);		//ADC_TO_DAC MUX Select ADC Left Channel
		CODEC_SetRoute(AUDIO_OUT_DEV_SPEAKER, 0);

		switch(device) {
			case AUDIO_IN_DEV_MAINMIC:
			case AUDIO_IN_DEV_LINEIN:
			case AUDIO_IN_DEV_DIGITALMIC:
				CODEC_SetRoute(device, 0);
				break;
			default:
				CODEC_DBG("invalid dev..\n");
				return -1;
		}
	}

	return 0;
}

static int32_t CODEC_Ioctl(AUDIO_Device dev, CODEC_ControlCmd cmd, uint32_t arg)
{
	int32_t ret = -1;

	if (AUDIO_IN_DEV_LINEIN == dev)
		return ret;

	switch (cmd) {
		case CODEC_CMD_SET_MIXSER:
			break;
		case CODEC_CMD_SET_ADDA_DIRECT:
			ret = CODEC_SetAdda_Direct(dev, arg);
			break;
		default:
			break;
	}

	return ret;
}

static int32_t CODEC_Open(CODEC_DataParam *param)
{
    if (!param)
		return -1;

	CODEC_DBG("CODEC open..\n");

	AUDIO_CODEC_Priv *priv = &gAudioCodecPriv;
	CODEC_DataParam *dataParam = param;

	if (dataParam->direction == CODEC_PLAYBACK) {
		if (priv->isTxInitiate == true) {
			CODEC_WRN("Tx dev opened already.\n");
			return 0;
		}
	} else {
		if (priv->isRxInitiate == true) {
			CODEC_WRN("Rx dev opened already.\n");
			return 0;
		}
	}

	if (dataParam->bufSize == 0) {
		CODEC_ERR("bufSize is 0.\n");
		return -1;
	}

	if (dataParam->direction == CODEC_PLAYBACK) {
		priv->txDMAChan = DMA_CHANNEL_INVALID;
		priv->txBufSize = dataParam->bufSize;
		priv->txHalfCallCount = 0;
		priv->txEndCallCount = 0;

		priv->txBuf = CODEC_MALLOC(priv->txBufSize);
		if(priv->txBuf)
			CODEC_MEMSET(priv->txBuf, 0, priv->txBufSize);
		else {
			CODEC_ERR("Malloc tx buf(for DMA),faild...\n");
			return -1;
		}

		/*request DMA channel*/
		priv->txDMAChan = HAL_DMA_Request();
		if (priv->txDMAChan == DMA_CHANNEL_INVALID) {
			CODEC_ERR("Obtain tx DMA channel,faild...\n");
			CODEC_FREE(priv->txBuf);
			return -1;
		} else
			CODEC_DMASet(priv->txDMAChan, CODEC_PLAYBACK);
		HAL_SemaphoreInitBinary(&priv->txReady);
		priv->isTxInitiate = true;
	} else {
		priv->rxDMAChan = DMA_CHANNEL_INVALID;
		priv->rxBufSize = dataParam->bufSize;
		priv->rxHalfCallCount = 0;
		priv->rxEndCallCount = 0;

		priv->rxBuf = CODEC_MALLOC(priv->rxBufSize);
		if(priv->rxBuf)
			CODEC_MEMSET(priv->rxBuf, 0, priv->rxBufSize);
		else {
			CODEC_ERR("Malloc rx buf(for DMA),faild...\n");
			return -1;
		}

		priv->rxDMAChan = HAL_DMA_Request();
		if (priv->rxDMAChan == DMA_CHANNEL_INVALID) {
			CODEC_ERR("Obtain rx DMA channel,faild...\n");
			CODEC_FREE(priv->rxBuf);
			return -1;
		} else
			CODEC_DMASet(priv->rxDMAChan, CODEC_RECORD);
		HAL_SemaphoreInitBinary(&priv->rxReady);
		priv->isRxInitiate = true;
	}

	CODEC_SET_InitCfg(dataParam);

	/*set sampleRate、resolution*/
    CODEC_SET_ClkDiv(dataParam);

	/*set channels*/
	CODEC_SET_Channels(dataParam);

	if (dataParam->direction == PLAYBACK) {
		CODEC_EnableTx();
	} else {
    	CODEC_EnableRx();
	}

	return 0;
}

static int32_t CODEC_Close(uint32_t dir)
{
    AUDIO_CODEC_Priv *priv = &gAudioCodecPriv;

	if (dir == CODEC_PLAYBACK) {
		if (priv->isTxInitiate != true) {
			CODEC_ERR("Tx dev opened already.\n");
			return -1;
		}
	} else {
		if (priv->isRxInitiate != true) {
			CODEC_ERR("Rx dev opened already.\n");
			return -1;
		}
	}

	CODEC_DBG("CODEC close..\n");

    if (dir == CODEC_PLAYBACK) {
        CODEC_DMA_Trigger(false,CODEC_PLAYBACK);
        CODEC_DisableTx();
        priv->txRunning = false;
        priv->isTxInitiate = false;
        if (priv->txDMAChan != DMA_CHANNEL_INVALID) {
			HAL_DMA_DeInit(priv->txDMAChan);
			HAL_DMA_Release(priv->txDMAChan);
			priv->txDMAChan = DMA_CHANNEL_INVALID;
        }
        //I2S_MEMSET(&(priv->pdataParam), 0, sizeof(I2S_DataParam));

        CODEC_FREE(priv->txBuf);

        HAL_SemaphoreDeinit(&priv->txReady);
        priv->txBuf = NULL;
        priv->txBufSize = 0;
        priv->writePointer = NULL;
        priv->txHalfCallCount = 0;
        priv->txEndCallCount = 0;
    } else {
        CODEC_DMA_Trigger(false,CODEC_RECORD);
        CODEC_DisableRx();
        priv->isRxInitiate = false;
        priv->rxRunning = false;
        if (priv->rxDMAChan != DMA_CHANNEL_INVALID) {
            HAL_DMA_DeInit(priv->rxDMAChan);
            HAL_DMA_Release(priv->rxDMAChan);
            priv->rxDMAChan = DMA_CHANNEL_INVALID;
        }
        //I2S_MEMSET(&(priv->cdataParam), 0, sizeof(I2S_DataParam));

        CODEC_FREE(priv->rxBuf);

        HAL_SemaphoreDeinit(&priv->rxReady);
        priv->rxBuf = NULL;
        priv->rxBufSize = 0;
        priv->readPointer = NULL;

        priv->rxHalfCallCount = 0;
        priv->rxEndCallCount = 0;
    }
    return 0;
}

static int32_t CODEC_Write_DMA(uint8_t *buf, uint32_t size)
{
    if (!buf || size <= 0)
            return HAL_INVALID;

    AUDIO_CODEC_Priv *priv = &gAudioCodecPriv;

    uint8_t *pdata = buf;
    uint8_t *lastWritePointer = NULL;
    uint32_t toWrite = 0, writeSize = priv->txBufSize / 2;
    uint8_t err_flag; /* temp solution to avoid outputing debug message when irq disabled */

	for ( ; size / writeSize; pdata += writeSize, toWrite += writeSize, size -= writeSize)
	{
		if (priv->txRunning == false) {
			if (!priv->writePointer)
			        priv->writePointer = priv->txBuf;
			lastWritePointer = priv->writePointer;

			CODEC_MEMCPY(lastWritePointer, pdata, writeSize);
			CODEC_DBG("Tx: play start...\n");
			CODEC_DMA_Trigger(true,CODEC_PLAYBACK);/*play*/
			priv->txRunning =true;
		} else {
			err_flag = 0;
			HAL_DisableIRQ();
			if (priv->txHalfCallCount && priv->txEndCallCount) {
				err_flag = 1;
				priv->txHalfCallCount = 0;
				priv->txEndCallCount = 0;
			} else if (priv->txHalfCallCount) {
				priv->txHalfCallCount --;
			} else if (priv->txEndCallCount) {
				priv->txEndCallCount --;
			} else {
				priv->isTxSemaphore = true;
				HAL_EnableIRQ();
				HAL_SemaphoreWait(&(priv->txReady), HAL_WAIT_FOREVER);
				HAL_DisableIRQ();
				if (priv->txHalfCallCount && priv->txEndCallCount) {
					err_flag = 1;
					priv->txHalfCallCount = 0;
					priv->txEndCallCount = 0;
				} else {
					if (priv->txHalfCallCount)
						priv->txHalfCallCount --;
					if (priv->txEndCallCount)
						priv->txEndCallCount --;
				}
			}

			if (priv->txDmaPointer == priv->txBuf) {
				lastWritePointer = priv->txBuf + writeSize;
				priv->writePointer = priv->txBuf;
			} else {
				lastWritePointer = priv->txBuf;
				priv->writePointer =  priv->txBuf + writeSize;
			}
			CODEC_MEMCPY(lastWritePointer, pdata, writeSize);
			HAL_EnableIRQ();

			if (err_flag) {
				CODEC_ERR("TxCount:(H:%u,F:%u)\n",priv->txHalfCallCount,
                                                  priv->txEndCallCount);
				CODEC_ERR("Tx : underrun....\n");
			}
		}
	}

    return toWrite;
}

static int32_t CODEC_Read_DMA(uint8_t *buf, uint32_t size)
{
    AUDIO_CODEC_Priv *priv = &gAudioCodecPriv;
    if (!buf || size <= 0)
            return HAL_INVALID;
    uint8_t *pdata = buf;
    uint8_t *lastReadPointer = NULL;
    uint32_t readSize = priv->rxBufSize / 2;
    uint32_t toRead = 0;
    uint8_t err_flag; /* temp solution to avoid outputing debug message when irq disabled */

	for ( ; size / readSize; pdata += readSize, toRead += readSize, size -= readSize) {
		if (priv->rxRunning == false) {
		    CODEC_DBG("Rx: record start...\n");
		    CODEC_DMA_Trigger(true,CODEC_RECORD);
		} else {
			err_flag = 0;
			/*disable irq*/
			HAL_DisableIRQ();
			lastReadPointer = priv->readPointer;
			if (priv->rxHalfCallCount && priv->rxEndCallCount) {
				err_flag = 1;
				priv->rxHalfCallCount = 0;
				priv->rxEndCallCount = 0;
			} else if (priv->rxHalfCallCount) {
				priv->rxHalfCallCount --;
			} else if (priv->rxEndCallCount) {
				priv->rxEndCallCount --;
			} else {
				/**enable irq**/
				priv->isRxSemaphore = true;
				HAL_EnableIRQ();
				HAL_SemaphoreWait(&(priv->rxReady), HAL_WAIT_FOREVER);
				/**disable irq**/
				HAL_DisableIRQ();
				if (priv->rxHalfCallCount && priv->rxEndCallCount) {
					err_flag = 1;
					priv->rxHalfCallCount = 0;
					priv->rxEndCallCount = 0;
				} else {
					if (priv->rxHalfCallCount)
						priv->rxHalfCallCount --;
					if (priv->rxEndCallCount)
						priv->rxEndCallCount --;
				}
			}

			if (priv->rxDmaPointer == priv->rxBuf) {
				lastReadPointer = priv->rxBuf + readSize;
			} else {
				lastReadPointer = priv->rxBuf;
			}
			CODEC_MEMCPY(pdata, lastReadPointer, readSize);
			//i2sPrivate->readPointer = lastReadPointer;
			/**enable irq**/
			HAL_EnableIRQ();
			if (err_flag) {
				//CODEC_ERR("Rx overrun, (H:%u,F:%u)\n",
				  //        priv->rxHalfCallCount,
				    //      priv->rxEndCallCount);
			}
		}
	}
    return toRead;
}

static int32_t CODEC_Init(void)
{
	AUDIO_CODEC_Priv *priv = &gAudioCodecPriv;

    if (priv->isHwInit == true)
        return 0;

	CODEC_DBG("CODEC init..\n");

	/*init and enable clk*/
	CODEC_AUDIOPLL_Update(AUD_PLL_24M);
	HAL_PRCM_SetAudioPLLParam(priv->audioPllParam);
	HAL_PRCM_EnableAudioPLL();

	HAL_CCM_BusEnablePeriphClock(CCM_BUS_PERIPH_BIT_AUDIO_CODEC);
	HAL_CCM_BusReleasePeriphReset(CCM_BUS_PERIPH_BIT_AUDIO_CODEC);
	HAL_CCM_AudioCodec_EnableMClock();

	CODEC_PowerOn();

	priv->isHwInit = true;
	return 0;
}

static int32_t CODEC_DeInit(void)
{
	AUDIO_CODEC_Priv *priv = &gAudioCodecPriv;

	if (priv->isHwInit != true) {
		return 0;
	}

	HAL_CCM_AudioCodec_DisableMClock();
	HAL_CCM_BusDisablePeriphClock(CCM_BUS_PERIPH_BIT_AUDIO_CODEC);
	HAL_PRCM_DisableAudioPLL();

	CODEC_PowerOff();

	priv->isHwInit = false;
	return 0;
}

struct codec_ctl_ops audio_codec_ctl_ops =  {
    .setRoute       = CODEC_SetRoute,
    .setVolume      = CODEC_SetVolume,
    .setTrigger     = NULL,
    .setEqScene     = NULL,
    .ioctl          = CODEC_Ioctl,
};

struct codec_drv_ops audio_codec_drv_ops = {
	.open	= CODEC_Open,
	.close	= CODEC_Close,
	.write 	= CODEC_Write_DMA,
	.read	= CODEC_Read_DMA,
	.init	= CODEC_Init,
	.deinit	= CODEC_DeInit,
};

AUDIO_CODEC AUDIOCODEC = {
	.ctl_ops	= &audio_codec_ctl_ops,
#if (__CONFIG_CHIP_ARCH_VER == 2)
	.drv_ops	= &audio_codec_drv_ops,
#endif
};

#endif /* (__CONFIG_CHIP_ARCH_VER == 2) */
