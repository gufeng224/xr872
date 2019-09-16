#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "audio/pcm/audio_pcm.h"
#include "audio/manager/audio_manager.h"
#include "driver/chip/codec.h"
#include "kernel/os/os.h"
#include "platform.h"
#include <pal_asound.h>
#include "../../example/audio_play/audio.h"
#include "common/apps/player_app.h"
#include "common/framework/platform_init.h"

#define AEC_THREAD_STACK_SIZE    (1024 * 4)

#define AUDIO_CARD_ID    AUDIO_CARD0




#define SAMPLE_SIZE         2
#define SAMPLE_16K_10MS     320   /* for 16 kHz */
#define SAMPLE_RATE         16000



struct audioContext {
	CODEC_INFO *codec;
	struct pcm_config pcm_cfg;
	unsigned int ReadLen;
	void *ReadData;
	short *MicSig;
	short *AecRef;
	short *LOut;
};
struct audioContext  pcm_raw_asr_config;
static int pcm_asound_count;
static OS_Semaphore_t sem;
static player_base *player;

extern const struct CodecCreatorS AC107CodecCtor;


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


static int audio_pcm_close(struct audioContext *pAudio)
{
	snd_pcm_close(AUDIO_CARD_ID, PCM_IN);
	audio_stop_ac107(pAudio);

	return 0;
}


static struct audioContext xr871_pcm_read_init(struct asound_resource *pasound_res)
{
    int ret;
    struct audioContext pAudio;

    ret = audio_start_ac107(&pAudio);

	/* enable 4 channel */
    aud_mgr_handler(AUDIO_DEVICE_MANAGER_PATH, AUDIO_IN_DEV_MAINMIC, 1);
    aud_mgr_handler(AUDIO_DEVICE_MANAGER_PATH, AUDIO_IN_DEV_LINEIN, 1);
    aud_mgr_handler(AUDIO_DEVICE_MANAGER_PATH, AUDIO_IN_DEV_DIGITALMIC, 1);

    pAudio.pcm_cfg.channels = 4;
    pAudio.pcm_cfg.format = 0;
    pAudio.pcm_cfg.period_count = 2;
    pAudio.pcm_cfg.period_size = 320;
    pAudio.pcm_cfg.rate = 16000;
    pAudio.pcm_cfg.mix_mode = 1;
    pAudio.ReadData = (void *)malloc(2560);
    pAudio.MicSig = (short *)malloc(320*2);
    ret = snd_pcm_open(&pAudio.pcm_cfg, AUDIO_CARD_ID, PCM_IN);
	
    return pAudio;
	

}



static int play_mp3_iis(void)
{
    int ret;
    player->set_callback(player, player_demo_callback, NULL);
    //ret = player->play(player, "flash://0?addr=981056&length=17626");  // 958k
    //ret = player->play(player, "flash://0?addr=1048640&length=17626");  //1024k
    //ret = player->play(player, "flash://0?addr=1286208&length=17626");  //1256k
    //ret = player->play(player, "flash://0?addr=2048064&length=17626");  //1256k
    //ret = player->play(player, "flash://0?addr=1117248&length=17626");  //1091k
    ret = player->play(player, "flash://0?addr=1090624&length=17626");  //1091k
    if (ret != 0) {
        printf("music play fail.\n");
        return -1;
    }
    OS_SemaphoreWait(&sem, OS_WAIT_FOREVER);

    player->stop(player);
   //  OS_SemaphoreDelete(&sem);
   // player_destroy(player);
	return 0;
}



static void xr872_mp3_iis_write_init(struct asound_resource *pasound_res)
{


    OS_Status ret;
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
	printf("player create success_+++++++++++.\n");
}


static int xr872_pcm_init(struct asound_resource *pasound_res)
{
    
    if(pasound_res->pcm_direction == YODALITE_PCM_OUT)
    {
     if(pasound_res->source == MP3_IIS)
     {
 	xr872_mp3_iis_write_init(pasound_res);
	return MP3_IIS;
     }
     else if(pasound_res->source == BT_SINK_IIS)
     {
        
	return BT_SINK_IIS;
     }
     else if (pasound_res->source == HTTP_IIS) 
     {
       
       return HTTP_IIS;
     }	  
    }
    if(pasound_res->pcm_direction == YODALITE_PCM_IN)
    {
     if(pasound_res->source == RAW_ASR)
     {
      pcm_raw_asr_config = xr871_pcm_read_init(pasound_res);
      return RAW_ASR;
     }
    }
    return 0;
}
static short buff_data[320];
static int xr872_pcm_read(int fd, void *data, unsigned int count)
{ 
   if (fd == RAW_ASR)
   {
    pcm_asound_count = snd_pcm_read(&pcm_raw_asr_config.pcm_cfg, AUDIO_CARD_ID, pcm_raw_asr_config.ReadData,count*4);
    for(int i =0;i<320;i++)
    {
     buff_data[i] = ((short *)pcm_raw_asr_config.ReadData)[4 * i + 2]; 
    }
   }
   data = buff_data;
   return pcm_asound_count;
}

static int xr872_pcm_write(int fd, void *data, unsigned int frame_cnt)
{
   if(fd == MP3_IIS)
   {
    fd = play_mp3_iis();
    return MP3_IIS;
   } else if (fd == HTTP_IIS) {
    return HTTP_IIS;
   }
   return 0;
}
static int xr872_pcm_ioctl(int fd, unsigned cmd, void *pvalue)
{
   return 0;
}



static int xr872_pcm_pause(int fd)
{
   if(fd ==HTTP_IIS)
   {
     
   }
   else if(fd == MP3_IIS)
   {
     
   }
   return 0;
}

static int xr872_pcm_resume(int fd)
{
   if(fd == HTTP_IIS)
   {
     
   }
   else if(fd == MP3_IIS)
   { 
     
   }
   return 0;
}

static int xr872_pcm_stop(int fd)
{
   if(fd == HTTP_IIS)
   {
    return fd;
   }
   else if(fd == MP3_IIS)
   {
    
    return fd;
   }
   return 0;
}

static int xr872_pcm_rerun(int fd)
{
   if(fd == HTTP_IIS)
   {
    
    return fd;
   }
   else if(fd == MP3_IIS)
   {
    
    return fd; 
   }
   return 0;
}

static int xr872_pcm_destroy(int fd)
{
   if(fd == MP3_IIS)
   {

     printf("destroy mp3 iis \n");
   }else if (fd == RAW_ASR) {
     
     printf("destory raw data\n");
   }else if (fd == HTTP_IIS) {
    
    printf("destroy http iis play +++++++\n");
   }
   return 0;
}

struct asound_lapi xr872_asound_lapi = {
    .yodalite_pcm_init = xr872_pcm_init,
    .yodalite_pcm_read = xr872_pcm_read,
    .yodalite_pcm_write = xr872_pcm_write,
    .yodalite_pcm_ioctl = xr872_pcm_ioctl,
    .yodalite_pcm_destroy = xr872_pcm_destroy,
    .yodalite_pcm_pause = xr872_pcm_pause,
    .yodalite_pcm_resume = xr872_pcm_resume,
    .yodalite_pcm_stop = xr872_pcm_stop,
    .yodalite_pcm_rerun = xr872_pcm_rerun,
};


