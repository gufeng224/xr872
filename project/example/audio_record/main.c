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
#include <string.h>
#include "common/framework/fs_ctrl.h"
#include "common/apps/recorder_app.h"
#include "common/framework/platform_init.h"


#include "audio/pcm/audio_pcm.h"
#include "audio/manager/audio_manager.h"
#include "driver/chip/codec.h"
//#include "debug.h"
#include "kws.h"

#define SOUND_CAPCARD    AUDIO_CARD2
static uint32_t sampleRate[] = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000};
static int AudioConfigIsValid(int samplerate, int channels)
{
	int i;
	int sr_num;

	if ((channels != 1) && (channels != 2))
		return 0;

	sr_num = sizeof(sampleRate) / sizeof(sampleRate[0]);
	for (i = 0;i < sr_num; i++) {
		if (sampleRate[i] == samplerate) {
			return 1;
		}
	}
	return 0;
}

static int AudioSetConfig(int samplerate, int channels, struct pcm_config *config)
{
	if (!AudioConfigIsValid(samplerate, channels)) {
		return -1;
	}

	config->channels = channels;
	config->rate = samplerate;
	config->period_size = 480;//2048;
	config->period_count = 2;
	config->format = PCM_FORMAT_S16_LE;
	config->mix_mode = 0;
	return 0;
}

static void record_demo(void)
{
#if 0
	recorder_base *recorder;
	rec_cfg cfg;

	if (fs_ctrl_mount(FS_MNT_DEV_TYPE_SDCARD, 0) != 0) {
		printf("mount fail\n");
		return;
	}

	recorder = recorder_create();
	if (recorder == NULL) {
		printf("recorder create fail, exit\n");
		return;
	}

	/* record a 15s amr media */
	cfg.type = XRECODER_AUDIO_ENCODE_AMR_TYPE;
	printf("start record amr now, last for 15s\n");
	recorder->start(recorder, "file://record/1.amr", &cfg);
	OS_Sleep(15);
	recorder->stop(recorder);
	printf("record amr over.\n");

	/* record a 15s pcm media */
	cfg.type = XRECODER_AUDIO_ENCODE_PCM_TYPE;
	cfg.sample_rate = 8000;
	cfg.chan_num = 1;
	cfg.bitrate = 12200;
	cfg.sampler_bits = 16;
	printf("start record pcm now, last for 15s\n");
	recorder->start(recorder, "file://record/1.pcm", &cfg);
	OS_Sleep(15);
	recorder->stop(recorder);
	printf("record pcm over.\n");

	recorder_destroy(recorder);
#endif
int samplerate;
	int channels;

	unsigned int pcm_buf_size;
	char *pcm_data;
	struct pcm_config config;
	samplerate = 16000;
		channels = 1;

if (AudioSetConfig(samplerate, channels, &config)) {
		printf("invalid audio cap param.\n");
		
	}


    pcm_buf_size = (config.channels)*2*(config.period_size);
    printf("pcm_buf_size : %d,period_size: %d\n",pcm_buf_size,config.period_size);
    pcm_data = malloc(pcm_buf_size);
    if (pcm_data == NULL) {
		printf("malloc buf failed\n");
    }
    memset(pcm_data, 0, pcm_buf_size);

    if (snd_pcm_open(&config, SOUND_CAPCARD, PCM_IN) != 0)
    {
		printf("sound card open err\n");
    }

float thresholdSetting[3] = {0.65, 0.40, 0.24};
   unsigned char channel_num = 1;
   unsigned char min_gap = 5;
   kws_task task = kws_create("123", channel_num, thresholdSetting, min_gap);
   static float *in_ptr[8];
   static float in_buffer[480];
   in_ptr[0] = in_buffer;
   //g_audio_task_end = 0;
   printf("asr run.\n");
   int	pcm_buff_count;
   while (1) {
		 pcm_buff_count =  snd_pcm_read(&config, SOUND_CAPCARD, pcm_data, pcm_buf_size);
	for (int i = 0; i < pcm_buff_count/2; i++) {
		  in_buffer[i]= (float)(((short *)pcm_data)[i]*10);
	}
	int active_status = kws_status(task, (const float ** )in_ptr, pcm_buff_count/2,NULL);
	if (active_status == 1) {
	  // player->set_callback(player, player_demo_callback, NULL);
   //player->play(player, "file://music/1.mp3");
   //OS_SemaphoreWait(&sem, OS_WAIT_FOREVER);
   //player->stop(player);
	   printf("wake ++++\n");
	   //snd_pcm_close(SOUND_CAPCARD, PCM_IN);
	   //snd_pcm_open(&config, SOUND_CAPCARD, PCM_IN);
	   //printf("open again sound_capcard\n");
	}
 }

}

int main(void)
{
	platform_init();

	printf("record demo start.\n");

	record_demo();

	printf("record demo over.\n");
	return 0;
}
