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

#include "common/apps/player_app.h"
#include <CdxTypes.h>
#include <stdlib.h>
#include <unistd.h>
#include "string.h"
#include "CdxFifoStream.h"
#include "kfifoqueue.h"
#include "audio.h"

typedef enum audio_status
{
    AUDIO_STATUS_PLAYING,
    AUDIO_STATUS_STOPPED,
    AUDIO_STATUS_ERROR,
} audio_status;

struct audio_controller {
    struct CdxFifoStreamS *fifobase;
    OS_Mutex_t   audio_mutex;
    OS_Thread_t  start_thread;
    audio_status status;
    PLAYER *player;
    unsigned int in_size;
    char has_play;
    char has_free;
};

static void audio_callback(player_events event, void *data, void *arg)
{
    struct audio_controller *controller;
    controller = (struct audio_controller *)arg;

    printf("player event:%d\n", event);

    switch (event) {
    case PLAYER_EVENTS_MEDIA_STOPPED:
    case PLAYER_EVENTS_MEDIA_ERROR:
    case PLAYER_EVENTS_MEDIA_PLAYBACK_COMPLETE:
        controller->status = AUDIO_STATUS_STOPPED;
        break;
    default:
        break;
    }
}

static int audio_controller_init(struct audio_controller *controller)
{
    controller->status = AUDIO_STATUS_STOPPED;
    controller->has_free = 1;
    if (OS_MutexCreate(&controller->audio_mutex) != OS_OK) {
        return -1;
    }
    return 0;
}

CONTROLLER *audio_controller_create()
{
    int ret;
    struct audio_controller *controller;

    controller = malloc(sizeof(*controller));
    if (controller == NULL) {
        printf("controller malloc fail\n");
        return NULL;
    }
    memset(controller, 0, sizeof(*controller));
    ret = audio_controller_init(controller);
    if (ret == -1) {
        free(controller);
        return NULL;
    }

    return (CONTROLLER *)controller;
}

void audio_controller_destroy(CONTROLLER *control)
{
    struct audio_controller *controller;
    controller = (struct audio_controller *)control;

    OS_MutexDelete((OS_Mutex_t *)&controller->audio_mutex);
    free(controller);
}

static int player_play_start(CONTROLLER *control, PLAYER *_player)
{
    char url[32];
    struct audio_controller *controller;
    player_base *player;

    controller = (struct audio_controller *)control;
    player = (player_base *)_player;

    sprintf(url, "fifo://%p", controller->fifobase);
    player->set_callback(player, audio_callback, controller);
    return player->play(player, (const char *)url);
}

static int player_play_stop(CONTROLLER *control, PLAYER *_player)
{
    player_base *player;
    player = (player_base *)_player;
    player->stop(player);
    return 0;
}

static void audio_play_task(void *arg)
{
    int ret;
    struct audio_controller *controller;

    controller = (struct audio_controller *)arg;

    ret = player_play_start(controller, controller->player);
    if (ret) {
        controller->status = AUDIO_STATUS_ERROR;
    }
    OS_ThreadDelete(&controller->start_thread);
}

int audio_play_start(CONTROLLER *control, PLAYER *player)
{
    struct audio_controller *controller;

    controller = (struct audio_controller *)control;

    OS_MutexLock(&controller->audio_mutex, OS_WAIT_FOREVER);
    if (controller->status == AUDIO_STATUS_PLAYING) {
        goto err;
    }

    controller->fifobase = kfifo_stream_create();
    if (controller->fifobase == NULL) {
        goto err;
    }

    controller->status = AUDIO_STATUS_PLAYING;
    controller->has_play = 0;
    controller->in_size = 0;
    controller->has_free = 0;
    OS_MutexUnlock(&controller->audio_mutex);

    return 0;

err:
    OS_MutexUnlock(&controller->audio_mutex);
    return -1;
}

int audio_play_put_data(CONTROLLER *control, PLAYER *player, const void * const inData, int dataLen)
{
    OS_Status ret;
    uint32_t avail;
    uint32_t in_len;
    uint32_t reserve_len;
    uint32_t has_in_len = 0;
    struct audio_controller *controller;

    controller = (struct audio_controller *)control;

    OS_MutexLock(&controller->audio_mutex, OS_WAIT_FOREVER);
    if (controller->status != AUDIO_STATUS_PLAYING) {
        OS_MutexUnlock(&controller->audio_mutex);
        return -1;
    }

    while ((has_in_len != dataLen) && (controller->status == AUDIO_STATUS_PLAYING)) {
        reserve_len = dataLen - has_in_len;
        CdxFifoStreamLock(controller->fifobase);
        avail = CdxFifoStreamAvail(controller->fifobase);
        in_len = avail > reserve_len ? reserve_len : avail;
        CdxFifoStreamIn(controller->fifobase, (char *)inData + has_in_len, in_len);
        has_in_len += in_len;
        controller->in_size += in_len;
        CdxFifoStreamUnlock(controller->fifobase);
        if ((controller->in_size >= (4 * 1024)) && (controller->has_play == 0)) {  /* it must be 4k */
            controller->player = player;
            ret = OS_ThreadCreate(&controller->start_thread, "", audio_play_task, controller, OS_THREAD_PRIO_APP, 1024);
            if (ret != OS_OK) {
                controller->status = AUDIO_STATUS_ERROR;
            }
            controller->has_play = 1;
        }
        if (has_in_len != dataLen)
            OS_MSleep(10);
    }
    OS_MutexUnlock(&controller->audio_mutex);
    return 0;
}


int audio_play_stop(CONTROLLER *control, PLAYER *player, bool stop_immediately)
{
    int ret;
    unsigned int waittime = 0;
    struct audio_controller *controller;

    controller = (struct audio_controller *)control;

    OS_MutexLock(&controller->audio_mutex, OS_WAIT_FOREVER);
    if (controller->status != AUDIO_STATUS_PLAYING) {
        goto err;
    }

    CdxFifoStreamSeteos(controller->fifobase);

    if (stop_immediately) {
        printf("stop immediately\n");
        player_play_stop(controller, player);
    } else {
        if (controller->has_play == 0) {
            ret = player_play_start(controller, player);
            if (ret) {
                controller->status = AUDIO_STATUS_ERROR;
            }
        }
        while ((controller->status == AUDIO_STATUS_PLAYING) && (CdxFifoStreamValid(controller->fifobase) != 0))
            OS_MSleep(100);
        while ((controller->status == AUDIO_STATUS_PLAYING) && (waittime < 5000)) {
            OS_MSleep(200);
            waittime += 200;
        }
        if (controller->status == AUDIO_STATUS_PLAYING)
            player_play_stop(controller, player);
    }

err:
    if (!controller->has_free) {
        controller->status = AUDIO_STATUS_STOPPED;
        controller->has_free = 1;
        kfifo_stream_destroy(controller->fifobase);
    }
    OS_MutexUnlock(&controller->audio_mutex);

    return 0;
}

