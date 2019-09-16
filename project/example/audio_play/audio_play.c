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
#include "kernel/os/os.h"
#include "pthread.h"
#include "common/framework/fs_ctrl.h"
#include "common/apps/player_app.h"
#include "common/framework/platform_init.h"
#include "fs/fatfs/ff.h"
#include "audio.h"
#include "pal_asound.h"
#include "platform.h"
#include "debug.h"
#include "kws.h"
#include "prj_config.h"
//#include "nsapi.h"
extern struct asound_lapi xr872_asound_lapi;

#define PLAYER_THREAD_STACK_SIZE    (1024 * 2)
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

static int play_file_music()
{
    int ret;

    /*
     * play media in sd/tf card.
     * 1. in this example, you should create a folder named music in you sd/tf card
     * 2. add 1.mp3 to this folder
     */
    player->set_callback(player, player_demo_callback, NULL);
    ret = player->play(player, "file://music/1.mp3");
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

static int play_flash_music()
{
    int ret;

    /*
     * play media in flash.
     * 1. in this example, we play 1.amr(in image/xr872)
     * 2. we should add 1.amr to our image.cfg, so we can write it to flash
     * 3. construct the url according to the offset and size of 1.amr.
     *    in this example, the offset of 1.amr is (1024 * 1024 + 64) = 1048640,
     *    and the size of 1.amr is 363750 bytes. so url is "flash://0?addr=1048640&length=363750"
     * be attention, we will add 64 bytes before the bin in image.cfg, so the offset of 1.amr is (1024 * 1024 + 64)
     */
    player->set_callback(player, player_demo_callback, NULL);
    ret = player->play(player, "flash://0?addr=1048640&length=363750");
    if (ret != 0) {
        printf("music play fail.\n");
        return -1;
    }
   
   uint8_t *start, *end, *current;
   
   heap_get_space(&start, &end, &current);
   printf("heap total %u (%u KB), use %u (%u KB), free %u (%u KB), [%p, %p, %p)\n",
							 end - start,(end - start)/1024,current - start,(current - start)/1024,end - current,(end - current)/1024 ,start, current, end);
   
    /* wait for playback complete */
    OS_SemaphoreWait(&sem, OS_WAIT_FOREVER);

    /* stop it */
    player->stop(player);
    return 0;
}

static int play_fifo_music()
{
    FIL fp;
    int ret = 0;
    FRESULT result;
    void *file_buffer;
    unsigned int act_read;
    CONTROLLER *controller;

    /*
     * play media by putting media data to player
     * 1. only support mp3/amr/wav
     */
    controller = audio_controller_create();
    if (controller == NULL) {
        return -1;
    }

    file_buffer = (void *)malloc(1024);
    if (file_buffer == NULL) {
        ret = -1;
        goto err1;
    }

    result = f_open(&fp, "music/1.mp3", FA_READ);
    if (result != FR_OK) {
        ret = -1;
        goto err2;
    }

    audio_play_start(controller, player);
    while (1) {
        f_read(&fp, file_buffer, 1024, &act_read);
        audio_play_put_data(controller, player, file_buffer, act_read);
        if (act_read != 1024)
            break;
    }
    audio_play_stop(controller, player, false);

    f_close(&fp);
err2:
    free(file_buffer);
err1:
    audio_controller_destroy(controller);
    return ret;
}

static void player_demo(void *arg)
{
    OS_Status ret;
    void *pcm_data;
    uint8_t *start, *end, *current;
    pal_asound_init(&xr872_asound_lapi);
    struct yodalite_pcm_config * config;
    unsigned int card, device, flags;
    config = (struct yodalite_pcm_config *)malloc(sizeof(struct yodalite_pcm_config));
    memset(config, 0x0, sizeof(struct yodalite_pcm_config));
    card = 1;
    device = 1;
    flags = YODALITE_PCM_IN;
    config->channels =1;
    config->rate = 16000;
    config->format = 16;
    config->period_count = sizeof(short);
    config->source = RAW_ASR;
    config->period_size = 320;
    int us_pcm_read = pcm_open(card, device, flags, config);
    printf("us_pcm_read+++%d\n",us_pcm_read);
    free(config);
    void *buff = NULL;
    float thresholdSetting[3] = {0.63,1.0,1.0}; 
    unsigned char channel_num = 1;
    unsigned char min_gap = 5;
    heap_get_space(&start, &end, &current);
    printf("heap total %u (%u KB), use %u (%u KB), free %u (%u KB), [%p, %p, %p)\n",
							 end - start,(end - start)/1024,current - start,(current - start)/1024,end - current,(end - current)/1024 ,start, current, end);
    kws_task task = kws_create("123", channel_num, thresholdSetting, min_gap);
    heap_get_space(&start, &end, &current);
    printf("heap total1111 %u (%u KB), use %u (%u KB), free %u (%u KB), [%p, %p, %p)\n",
							 end - start,(end - start)/1024,current - start,(current - start)/1024,end - current,(end - current)/1024 ,start, current, end);
#ifdef NS
    int nsMode = 0 ;
    static Ns * nsPtr;
    nsPtr = ns_create(16000, nsMode);
    if(ns_init(nsPtr) != 0) {
	ns_destroy(nsPtr);
	printf("ns_init failed\n");
	return 0;
    }
    heap_get_space(&start, &end, &current);
    printf("heap total %u (%u KB), use %u (%u KB), free %u (%u KB), [%p, %p, %p)\n",
							 end - start,(end - start)/1024,current - start,(current - start)/1024,end - current,(end - current)/1024 ,start, current, end);
    record_output_init1();
#endif
    record_output_init();
    pcm_data = psram_malloc(2560);
    static float *in_ptr[8];
    static float in_buffer[320];//in_buffer[480];
    static short pcm_out_data[320];
    static short ns_buffer[320];
    in_ptr[0] = in_buffer;
    int i;
    while(1)
    {
     pcm_read(us_pcm_read,  (void *)pcm_data, 2560);
     for (int i = 0; i < 320; i++) {
	pcm_out_data[i] = ((short *)pcm_data)[4 * i + 2];
      }
#ifdef NS
     int buffer_length_ns = 0;
     buffer_length_ns = ns_process(nsPtr, pcm_out_data, 320, ns_buffer, 640);
     for (int i = 0; i < 320; i++) {
           in_buffer[i]= (float)(ns_buffer[i]*20);
    }
     
     record_output_put_data1(ns_buffer, 640);
     record_output_put_data(pcm_out_data, 640);
#elif KWS
     record_output_put_data(pcm_out_data, 640);
     for (int i = 0; i < 320; i++) {
           in_buffer[i]=(float)(pcm_out_data[i]*20);
    }
#endif
      int active_status =kws_status(task, (const float ** )in_ptr, 320,NULL);
      if (active_status == 1)
	  {
  

static char buf[1024];  
vTaskGetRunTimeStats(buf);           
printf("Run Time Stats:\nTask Name    Time    Percent\n%s\n", buf);
 
           heap_get_space(&start, &end, &current);
            printf("heap total %u (%u KB), use %u (%u KB), free %u (%u KB), [%p, %p, %p)\n",
							 end - start,(end - start)/1024,current - start,(current - start)/1024,end - current,(end - current)/1024 ,start, current, end);
	    printf("wake up+++\n");
	  //	pcm_write(mp3_audio_out,  (void *)buff,   1);
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
    OS_ThreadDelete(&player_thread);
}
int player_thread_init()
{
       if (OS_ThreadCreate(&player_thread,
                        "player_task",
                        player_demo,
                        NULL,
                        OS_THREAD_PRIO_APP,
                        PLAYER_THREAD_STACK_SIZE) != OS_OK) {
        printf("thread create fail.exit\n");  
      return -1;
    }
//(void) pthread_create(&player_thread, NULL,player_demo,NULL);
    return 0;
}

