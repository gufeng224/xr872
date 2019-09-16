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

#include "debug.h"

#if SAVE_TO_SDCARD

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fs/fatfs/ff.h"
#include "common/framework/fs_ctrl.h"
#include "kernel/os/os_time.h"

#define SAVE_RECORD_DATA_DURATION_MS   10000

struct outputContext {
    FIL file;
    int status;
    unsigned int startTime;
};

static struct outputContext outContext;

int record_output_init()
{
    unsigned int tick;
    struct outputContext *context = &outContext;

    memset(context, 0, sizeof(*context));

    if (fs_ctrl_mount(FS_MNT_DEV_TYPE_SDCARD, 0) != 0) {
        printf("mount fail\n");
    }
    f_unlink("record/1.pcm");
    f_open(&context->file, "record/1.pcm", FA_CREATE_ALWAYS | FA_READ | FA_WRITE);

    tick = OS_GetTicks();
    context->startTime = OS_TicksToMSecs(tick);
    context->status = 1;

    printf("===start save record data, and we only save %dms===\n", SAVE_RECORD_DATA_DURATION_MS);

    return 0;
}

int record_output_put_data(void *data, unsigned int len)
{
    unsigned int tick;
    unsigned int nowTime;
    unsigned int writeLen;
    struct outputContext *context = &outContext;

    if (context->status == 0) {
        return 0;
    }

    f_write(&context->file, data, len, &writeLen);
    tick = OS_GetTicks();
    nowTime = OS_TicksToMSecs(tick);
    if ((nowTime - context->startTime) > SAVE_RECORD_DATA_DURATION_MS) {
        f_close(&context->file);
        context->status = 0;
        printf("===stop save record data===\n");
    }
    return writeLen;
}

int record_output_deinit()
{
    struct outputContext *context = &outContext;

    if (context->status == 1) {
        f_close(&context->file);
        context->status = 0;
        printf("===stop save record data===\n");
    }
    return 0;
}

#elif SAVE_TO_FLASH

#include <stdio.h>
#include <string.h>
#include "driver/chip/hal_flash.h"

#define FLASH_START_ADDR  (2048 * 1024)
#define FLASH_DEVICE_NUM  0
#define SAVE_BUFFER_LEN   (128 * 1024 * 4)

struct outputContext {
    int validLen;
    int status;
};

static struct outputContext outContext;

int record_output_init()
{
    uint32_t blk_cnt;
    struct outputContext *context = &outContext;

    context->validLen = 0;
    context->status = 1;

    if (HAL_Flash_Open(FLASH_DEVICE_NUM, 3000) != HAL_OK) {
        printf("flash open fail\n");
        context->status = 0;
        return -1;
    }
    blk_cnt = SAVE_BUFFER_LEN / (1024 * 4);
    HAL_Flash_Erase(FLASH_DEVICE_NUM, FLASH_ERASE_4KB, FLASH_START_ADDR, blk_cnt);

    printf("===start save record data, and we only save %dbytes===\n", SAVE_BUFFER_LEN);

    return 0;
}

int record_output_put_data(void *data, unsigned int len)
{
    int cpy_len;
    int avail_len;
    uint32_t write_addr;
    struct outputContext *context = &outContext;

    if (context->status == 0) {
        return 0;
    }

    avail_len = SAVE_BUFFER_LEN - context->validLen;
    cpy_len = len > avail_len ? avail_len : len;
    write_addr = FLASH_START_ADDR + context->validLen;
    HAL_Flash_Write(FLASH_DEVICE_NUM, write_addr, (uint8_t *)data, cpy_len);
    context->validLen += cpy_len;
    if (context->validLen == SAVE_BUFFER_LEN) {
        context->status = 0;
        HAL_Flash_Close(FLASH_DEVICE_NUM);
        printf("===stop save record data===\n");
    }
    return len;
}

int record_output_deinit()
{
    struct outputContext *context = &outContext;

    if (context->status == 1) {
        context->status = 0;
        HAL_Flash_Close(FLASH_DEVICE_NUM);
        printf("===stop save record data===\n");
    }
    return 0;
}

#endif
