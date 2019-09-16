/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
G*    1. Redistributions of source code must retain the above copyright
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "kernel/os/os_thread.h"
#include "FreeRTOS.h"
#include "task.h"
#include "audio/pcm/audio_pcm.h"
#include "audio/manager/audio_manager.h"
#include "driver/chip/codec.h"
#include "debug.h"
#include "kws.h"
#include "kernel/os/os.h"
#include "common/framework/fs_ctrl.h"
#include "common/apps/player_app.h"
#include "common/framework/platform_init.h"
#include "fs/fatfs/ff.h"
#include "../example/audio_play/audio.h"
#include "pal_asound.h"
#include "platform.h"

#define AEC_SUPPORT
#define ASR_SUPPORT
extern struct asound_lapi xr872_asound_lapi;

#ifdef AEC_SUPPORT
//TODO:add head file of aec algorithm
#endif

#ifdef ASR_SUPPORT
//TODO:add head file of asr algorithm
#endif

#define SAMPLE_SIZE         2
#define SAMPLE_16K_10MS     320   /* for 16 kHz */
#define SAMPLE_RATE         16000

#define AUDIO_CARD_ID    AUDIO_CARD0

#define AEC_THREAD_STACK_SIZE    (1024 * 4)

#ifdef ASR_SUPPORT
struct AsrContext {
	//TODO:add value of asr algorithm
	int keyword;
};
#endif

#ifdef AEC_SUPPORT
struct AecContext {
	//TODO:add value of aec algorithm
	int reserve;
};
#endif

struct audioContext {
	CODEC_INFO *codec;
	struct pcm_config pcm_cfg;
	unsigned int ReadLen;
	void *ReadData;
	short *MicSig;
	short *AecRef;
	short *LOut;
};

static OS_Thread_t aec_thread;

#ifdef ASR_SUPPORT
static int Asr_Init(struct AsrContext *pAsr)
{
	memset(pAsr, 0, sizeof(*pAsr));

	//TODO:init asr algorithm.0 for success, -1 for fail.
	return 0;
}

static inline int Asr_ProcessSample(struct AsrContext *pAsr, void *sample, unsigned int len)
{
	//TODO:process a sample.
	//and you can save recognition result in value "keyword"
	return 0;
}

static void Asr_Deinit(struct AsrContext *pAsr)
{
	//TODO:deinit asr algorithm
}
#endif

#ifdef AEC_SUPPORT
static int Aec_Init(struct AecContext *pAec)
{
	memset(pAec, 0, sizeof(*pAec));

	//TODO:init asr algorithm.0 for success, -1 for fail.
	return 0;
}

static inline int Aec_ProcessTx(struct AecContext *pAec, short *MicSig, short *AecRef, short *LOut)
{
	//TODO:process a sample.
	//input: micsig and aecref
	//output: lout
	return 0;
}

static void Aec_Deinit(struct AecContext *pAec)
{
	//TODO:deinit asr algorithm
}
#endif

static OS_Thread_t player_thread;
static OS_Semaphore_t sem;
static player_base *player;

static void player_demo_callback(player_events event, void *data, void *arg)
{
    switch (event) {
    case PLAYER_EVENTS_MEDIA_PREPARED:
        printf("media is prepared.\n");
        printf("you can use player to seek, get duration(by size), get current position(by tell) from now on.\n");
        break;
    case PLAYER_EVENTS_MEDIA_STOPPED:
        printf("media is stopped by user.\n");
        break;
    case PLAYER_EVENTS_MEDIA_ERROR:
        printf("error occur\n");
        OS_SemaphoreRelease(&sem);
        break;
    case PLAYER_EVENTS_MEDIA_PLAYBACK_COMPLETE:
        printf("media play is complete\n");
        OS_SemaphoreRelease(&sem);
        break;
    default:
        break;
    }
}


static short pcm_data[320];
extern const struct CodecCreatorS AC107CodecCtor;
static int audio_start_ac107(struct audioContext *pAudio)
{
	struct CodecProbeParam probeParam;
	struct CodecProbeResult result;
	struct CodecCreateParam createParam;
	const struct CodecCreatorS *codecCtor;
	CODEC_HWParam ac107_hwParam = {
		.speaker_double_used = 1,
		.double_speaker_val  = 0x10,
		.single_speaker_val  = 0x10,
		.single_speaker_ch   = CODEC_RIGHT,
		.headset_val         = 0x2b,
		.mainmic_type        = CODEC_MIC_ANALOG,
		.mainmic_analog_val  = 0x1f,
		.mainmic_digital_val = 0xa0,
		.headsetmic_val      = 0x1f,
	};

	/* copy from hal_codec.c */
	const DAI_FmtParam gFmtParam = {
		DAIFMT_CBS_CFS,
		DAIFMT_I2S,
		DAIFMT_NB_NF,
		32, 		   /*16,32,64,128,256*/
		16,
		24576000,
		24576000,
		MCLK1
	};

	codecCtor = &AC107CodecCtor;
	probeParam.i2cId = I2C0_ID;
	if (codecCtor->probe(&probeParam, &result) != HAL_OK) {
		return -1;
	}
	createParam.i2cId = result.i2cId;
	createParam.i2cAddr = result.i2cAddr;
	createParam.read = HAL_I2C_Master_Receive_Mem_IT;
	createParam.write = HAL_I2C_Master_Transmit_Mem_IT;
	pAudio->codec = codecCtor->create(&createParam);
	if (pAudio->codec == NULL) {
		return -1;
	}

	pAudio->codec->ops->setInitParam(HAL_CODEC_INIT, (void *)&ac107_hwParam);
	pAudio->codec->ops->setPower(HAL_CODEC_INIT, NULL);
	pAudio->codec->ops->setSysClk(HAL_CODEC_INIT, NULL);
	pAudio->codec->ops->jackDetect(HAL_CODEC_INIT, NULL);
	pAudio->codec->dai_ops->setPll(&gFmtParam);
	pAudio->codec->dai_ops->setClkdiv(&gFmtParam, SAMPLE_RATE);
	pAudio->codec->ctl_ops->setRoute(AUDIO_IN_DEV_MAINMIC, 1);

	return 0;
}

static int audio_stop_ac107(struct audioContext *pAudio)
{
	pAudio->codec->dai_ops->shutDown(0, 0);
	return 0;
}

static int audio_pcm_open(struct audioContext *pAudio)
{
	int ret;

	ret = audio_start_ac107(pAudio);
	if (ret) {
		printf("AC107 start fail.\n");
		return -1;
	}

	/* enable 4 channel */
	aud_mgr_handler(AUDIO_DEVICE_MANAGER_PATH, AUDIO_IN_DEV_MAINMIC, 1);
	aud_mgr_handler(AUDIO_DEVICE_MANAGER_PATH, AUDIO_IN_DEV_LINEIN, 1);
	aud_mgr_handler(AUDIO_DEVICE_MANAGER_PATH, AUDIO_IN_DEV_DIGITALMIC, 1);

	pAudio->pcm_cfg.channels = 4;
	pAudio->pcm_cfg.format = PCM_FORMAT_S16_LE;
	pAudio->pcm_cfg.period_count = 2;
	pAudio->pcm_cfg.period_size = SAMPLE_16K_10MS;
	pAudio->pcm_cfg.rate = SAMPLE_RATE;
	pAudio->pcm_cfg.mix_mode = 1;

	ret = snd_pcm_open(&pAudio->pcm_cfg, AUDIO_CARD_ID, PCM_IN);
	if (ret != 0) {
		printf("pcm open error\n");
		audio_stop_ac107(pAudio);
		return -1;
	}

	return 0;
}

static int audio_pcm_close(struct audioContext *pAudio)
{
	snd_pcm_close(AUDIO_CARD_ID, PCM_IN);
	audio_stop_ac107(pAudio);

	return 0;
}


static int audio_record_start(struct audioContext *pAudio)
{
	int ret = 0;
	int len = 0;

	memset(pAudio, 0, sizeof(*pAudio));

	ret = audio_pcm_open(pAudio);
	if (ret) {
		return -1;
	}

	pAudio->ReadLen = SAMPLE_16K_10MS * SAMPLE_SIZE * 4;
	pAudio->ReadData = (void *)malloc(pAudio->ReadLen);
	if (pAudio->ReadData == NULL) {
		goto err;
	}

	

	return 0;

err:
	free(pAudio->ReadData);
	audio_pcm_close(pAudio);
	return -1;
}

static int audio_get_micsig_aecref(struct audioContext *pAudio)
{
	int i;
	int ret;
	int len = 0;

	ret = snd_pcm_read(&pAudio->pcm_cfg, AUDIO_CARD_ID, pAudio->ReadData, pAudio->ReadLen);
	if (ret != pAudio->ReadLen) {
		printf("snd_pcm_read fail.\n");
		return -1;
	}

	/* according to your circuit and your aec algorithm */
#if 0//noted
	for (i = 0; i < SAMPLE_16K_10MS; i++) {
		pAudio->MicSig[i] = ((short *)pAudio->ReadData)[4 * i + 2];
		pAudio->MicSig[i + SAMPLE_16K_10MS] = ((short *)pAudio->ReadData)[4 * i + 3];
		pAudio->AecRef[i] = ((short *)pAudio->ReadData)[4 * i + 1];
	}
	//memcpy(pcm_data,pAudio->MicSig,1280);
	record_output_put_data(pAudio->ReadData, pAudio->ReadLen);
#endif
#if 0//noted
	for (i = 0; i < SAMPLE_16K_10MS; i++) {
		pAudio->MicSig[i*2] = ((short *)pAudio->ReadData)[4 * i + 2];
		pAudio->MicSig[i*2 + 1] = ((short *)pAudio->ReadData)[4 * i + 3];
		pAudio->AecRef[i] = ((short *)pAudio->ReadData)[4 * i + 1];
	}
	//memcpy(pcm_data,pAudio->MicSig,1280);
	record_output_put_data(pAudio->ReadData, pAudio->ReadLen);
#endif

#if 0

	for (i = 0; i < SAMPLE_16K_10MS; i++) {
		pcm_data[i] = ((short *)pAudio->ReadData)[4 * i + 2];
	}



    
	record_output_put_data(pcm_data, 640);
#endif

	return 0;
}

static void audio_record_stop(struct audioContext *pAudio)
{
	free(pAudio->LOut);
	free(pAudio->MicSig);
	free(pAudio->AecRef);
	free(pAudio->ReadData);
	audio_pcm_close(pAudio);
}

#ifdef ASR_SUPPORT
static void wakeup_result_process(struct audioContext *pAudio, int isKeyword)
{
	if (isKeyword) {
		printf("keyword recognized success.\n");
	}
}
#endif

static int play_flash()
{
    int ret;


    player->set_callback(player, player_demo_callback, NULL);
    ret = player->play(player, "flash://0?addr=1090624&length=17626");
    if (ret != 0) {
        printf("music play fail.\n");
        return -1;
    }

    /* wait for playback complete */
    OS_SemaphoreWait(&sem, OS_WAIT_FOREVER);

    /* stop it */
    player->stop(player);
    return 0;
}


#if 0
static void aec_asr_task(void *arg)
{
	int ret;
	struct audioContext aContext;

	ret = OS_SemaphoreCreate(&sem, 0, OS_SEMAPHORE_MAX_COUNT);
    if (ret != OS_OK) {
        printf("sem create fail\n");
        return;
    }

    player = player_create();
    if (player == NULL) {
        printf("player create fail.\n");
        OS_SemaphoreDelete(&sem);
        return;
    }

    printf("player create success.\n");
    printf("you can use it to play, pause, resume, set volume and so on.\n");
/*
    while (1) {
        play_flash();
    }

    OS_SemaphoreDelete(&sem);
    player_destroy(player);
*/
#if 1
	ret = audio_record_start(&aContext);
	if (ret) {
		goto exit2;
	}

	float thresholdSetting[3] = {0.63, 1.0, 1.0};   
	unsigned char channel_num = 1;	
	unsigned char min_gap = 5;	
	kws_task task = kws_create("123", channel_num, thresholdSetting, min_gap);	
	static float *in_ptr[8];	
	static float in_buffer[320];//in_buffer[480];	
	in_ptr[0] = in_buffer;	
	static int asr_num; 
        void * data_buff = malloc(640);
        record_output_init();    
	while (1) {
		//audio_get_micsig_aecref(&aContext);


	ret = snd_pcm_read(&aContext.pcm_cfg, AUDIO_CARD_ID, aContext.ReadData, aContext.ReadLen);
	if (ret != aContext.ReadLen) {
		printf("snd_pcm_read fail.\n");
		return -1;
	}

	

	for (int i = 0; i < SAMPLE_16K_10MS; i++) {
		pcm_data[i] = ((short *)aContext.ReadData)[4 * i + 2];
	//	((short *)data_buff)[i] = ((short *)aContext.ReadData)[4 * i + 2];
	}
       data_buff = pcm_data;
       //memcpy(data_buff,pcm_data,640);
       record_output_put_data(data_buff, 640);

	for (int i = 0; i < 320; i++) 
	{ 		  
	 in_buffer[i]=(float)(((short *)data_buff)[i]*10);// (float)(pcm_data[i]*20);	 
	}	   
	int active_status = kws_status(task, (const float ** )in_ptr, 320,NULL); 	
	if (active_status == 1) 
		{	  
		printf("wake up ++++%d\n",asr_num++);	   
        //play_flash();
uint8_t *start, *end, *current;
   
           heap_get_space(&start, &end, &current);
            printf("heap total %u (%u KB), use %u (%u KB), free %u (%u KB), [%p, %p, %p)\n",
							 end - start,(end - start)/1024,current - start,(current - start)/1024,end - current,(end - current)/1024 ,start, current, end);
        		
}


    
	

	}

	audio_record_stop(&aContext);
exit2:
#endif
	OS_ThreadDelete(&aec_thread);
}
#endif

#if  1
static void aec_asr_task(void *arg)
{

int mp3_audio_out;
    OS_Status ret;
	void *pcm_data1;

    /*if (fs_ctrl_mount(FS_MNT_DEV_TYPE_SDCARD, 0) != 0) {
        printf("mount fail\n");
        return;
    }*/
	pal_asound_init(&xr872_asound_lapi);
	struct yodalite_pcm_config * config;
	unsigned int card, device, flags;
	config = (struct yodalite_pcm_config *)malloc(sizeof(struct yodalite_pcm_config));
	memset(config, 0x0, sizeof(struct yodalite_pcm_config));
	card = 2;
	device = 2;
	flags = YODALITE_PCM_OUT;
	config->format = 16;
	config->channels = 2;
	config->rate = 16000;
	config->period_count = sizeof(short);
	config->source = MP3_IIS;
	config->file_source = AWEAK;
	mp3_audio_out = pcm_open(card, device, flags, config);
	printf("mp3_audio_out+++%d\n",mp3_audio_out);
	void *buff = NULL;

	card = 1;
    device = 1;
    flags = YODALITE_PCM_IN;
    config->channels = 1;
    config->rate = 16000;
    config->format = 16;
    config->period_count = sizeof(short);
    config->source = RAW_ASR;
    config->period_size = 320;
    int us_pcm_read = pcm_open(card, device, flags, config);
	printf("us_pcm_read+++%d\n",us_pcm_read);
	
	float thresholdSetting[3] = {0.65, 0.40, 0.24};  
    unsigned char channel_num = 1;
    unsigned char min_gap = 5;
    kws_task task = kws_create("123", channel_num, thresholdSetting, min_gap);
    //pcm_data1 = psram_malloc(2560);
    pcm_data1 = psram_malloc(640);

	 if (NULL == pcm_data1) {
        printf("Memory allocation failed!\n");
        kws_destroy(task);
        task = NULL;
		psram_free(pcm_data1);
        return;
    }
    static float *in_ptr[8];
    static float in_buffer[320];//in_buffer[480];
    static short pcm_out_data[320];
   // static char task_buff[1024];
    in_ptr[0] = in_buffer;
	int  pcm_buff_count;
	int i;
    while(1)
    {
   

//	  	pcm_write(mp3_audio_out,  (void *)buff,   1);
	pcm_buff_count =  pcm_read(us_pcm_read,  (void *)pcm_data1, 2560);
      /*for (int i = 0; i < 320; i++) {
		pcm_out_data[i] = ((short *)pcm_data1)[4 * i + 2];
	  }*/
	  for (int i = 0; i < 320; i++) {
           in_buffer[i]= (float)(((short *)pcm_data1)[i]*10);//(float)(pcm_data1[i]*20);
      }
      int active_status = kws_status(task, (const float ** )in_ptr, 320,NULL);
      if (active_status == 1)
	  {
	    printf("wake up+++\n");
     //       OS_ThreadList();
            //vTaskGetRunTimeStats(task_buff);
           // printf("run time stats:\n task name time percent\n%s\n",task_buff);
//	  	pcm_write(mp3_audio_out,  (void *)buff,   1);
      }
    }


	
/*
    ret = OS_SemaphoreCreate(&sem, 0, OS_SEMAPHORE_MAX_COUNT);
    if (ret != OS_OK) {
        printf("sem create fail\n");
        return;
    }
    player = player_create();
    if (player == NULL) {
        printf("player create fail.\n");
        OS_SemaphoreDelete(&sem);
        return;
    }

    printf("player create success.\n");
    printf("you can use it to play, pause, resume, set volume and so on.\n");

    while (1) {
        //play_file_music();
        play_flash_music();
        //play_fifo_music();
    }
    OS_SemaphoreDelete(&sem);

    player_destroy(player);
*/
    OS_ThreadDelete(&aec_thread);
}
#endif
int aec_asr_start()
{
	if (OS_ThreadCreate(&aec_thread,
                        "aec_task",
                        aec_asr_task,
                        NULL,
                        OS_THREAD_PRIO_APP,
                        AEC_THREAD_STACK_SIZE) != OS_OK) {
		printf("thread create error\n");
		return -1;
	}
	return 0;
}

