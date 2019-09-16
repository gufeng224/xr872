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

#ifndef SOUND_STREAM_CONTROL_H
#define SOUND_STREAM_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void * SoundStreamCtrl;

typedef enum {
	STREAM_TYPE_SOUND_CARD   = 0,
	STREAM_TYPE_REVERB_PCM   = 1,
	STREAM_TYPE_MAX_VALUE    = 2,
} SoundStreamType;

typedef enum {
	STREAM_CMD_SET_CONFIG,
	STREAM_CMD_SET_OUTPUT_CONFIG,
	STREAM_CMD_CLEAR_OUTPUT_CONFIG,
	STREAM_CMD_SET_BLOCK_MODE,
} SoundStreamCmd;

struct SscPcmConfig {
	unsigned int  channels;
	unsigned int  rate;
};

SoundStreamCtrl snd_stream_create(SoundStreamType type);
void snd_stream_destroy(SoundStreamCtrl ssc, SoundStreamType type);
int snd_stream_control(SoundStreamCtrl ssc, SoundStreamType type, SoundStreamCmd cmd, void *param);
int snd_stream_open(SoundStreamCtrl ssc, SoundStreamType type);
int snd_stream_close(SoundStreamCtrl ssc, SoundStreamType type);
int snd_stream_flush(SoundStreamCtrl ssc, SoundStreamType type);
int snd_stream_write(SoundStreamCtrl ssc, SoundStreamType type, void* pData, int nDataSize);
int snd_stream_read(SoundStreamCtrl ssc, SoundStreamType type, void* pData, int nDataSize);

#ifdef __cplusplus
}
#endif

#endif