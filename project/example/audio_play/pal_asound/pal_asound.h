/* pal_asound.h
**
** Copyright 2011, The Android Open Source Project
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of The Android Open Source Project nor the names of
**       its contributors may be used to endorse or promote products derived
**       from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY The Android Open Source Project ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL The Android Open Source Project BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
** DAMAGE.
*/
#ifndef _PAL_ASOUND_H_
#define _PAL_ASOUND_H_

#include <sys/time.h>

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * PCM API
 */

#define YODALITE_PCM_OUT        0x00000001
#define YODALITE_PCM_IN         0x10000000
#define PCM_MMAP       0x00000002
#define PCM_NOIRQ      0x00000004
/* PCM runtime states */
#define	PCM_STATE_OPEN		0
#define	PCM_STATE_SETUP		1
#define	PCM_STATE_PREPARED	2
#define	PCM_STATE_RUNNING		3
#define	PCM_STATE_XRUN		4
#define	PCM_STATE_DRAINING	5
#define	PCM_STATE_PAUSED		6
#define	PCM_STATE_SUSPENDED	7
#define	PCM_STATE_DISCONNECTED	8

#define PCM_ERROR_MAX 128
#define PCM_PATH "snd_card"

#define SNDRV_PCM_CONFIG_IOCTL 0x01

/* Bit formats */
enum yodalite_pcm_format {
    YODA_PCM_FORMAT_S16_LE = 0,
    YODA_PCM_FORMAT_S32_LE,
    YODA_PCM_FORMAT_MAX,
};

enum pcm_source{
    BT_SINK_IIS = 0,
    HTTP_IIS,
    MP3_IIS,
    SD_IIS,
    RAW_ASR,
    RAW_SD,
};
enum mp3_source{
    AWEAK = 0,
    PLAY_ERR,
    START_UP,
};
/* Configuration for a stream */
struct yodalite_pcm_config {
    unsigned int channels;
    unsigned int rate;
    unsigned int period_size;
    unsigned int period_count;
    enum yodalite_pcm_format format;
    enum pcm_source source;
    enum mp3_source file_source;
    char *url_str;
    /* Values to use for the ALSA start, stop and silence thresholds.  Setting
     * any one of these values to 0 will cause the default tinyalsa values to be
     * used instead.  Tinyalsa defaults are as follows.
     *
     * start_threshold   : period_count * period_size
     * stop_threshold    : period_count * period_size
     * silence_threshold : 0
     */
    unsigned int start_threshold;
    unsigned int stop_threshold;
    unsigned int silence_threshold;
    /* Minimum number of frames available before pcm_mmap_write() will actually
     * write into the kernel buffer. Only used if the stream is opened in mmap mode
     * (pcm_open() called with PCM_MMAP flag set).   Use 0 for default.
     */
    int avail_min;
};

struct yodalite_pcm {
    int fd;
    unsigned int flags;
    int running:1;
    int underruns;
    unsigned int buffer_size;
    unsigned int boundary;
    char error[PCM_ERROR_MAX];
    struct yodalite_pcm_config config;
/*
    struct snd_pcm_mmap_status *mmap_status;
    struct snd_pcm_mmap_control *mmap_control;
    struct snd_pcm_sync_ptr *sync_ptr;
*/
    void *mmap_buffer;
    unsigned int noirq_frames_per_msec;
    int wait_for_avail_min;
};

/* Mixer control types */
enum mixer_ctl_type {
    MIXER_CTL_TYPE_BOOL,
    MIXER_CTL_TYPE_INT,
    MIXER_CTL_TYPE_ENUM,
    MIXER_CTL_TYPE_BYTE,
    MIXER_CTL_TYPE_IEC958,
    MIXER_CTL_TYPE_INT64,
    MIXER_CTL_TYPE_UNKNOWN,
    MIXER_CTL_TYPE_MAX,
};

struct asound_resource {
    unsigned asound_id;
    unsigned channels;
    unsigned sample_rate;
    enum yodalite_pcm_format format;
    unsigned pcm_direction;
    enum pcm_source source;
    enum mp3_source file_source;
    char* url_str;
};

struct asound_lapi {
    int (*yodalite_pcm_init)(struct asound_resource *pasound_res);
    int (*yodalite_pcm_read)(int fd, void *data, unsigned int frame_cnt);
    int (*yodalite_pcm_write)(int fd, void *data, unsigned int frame_cnt);
    int (*yodalite_pcm_ioctl)(int fd, unsigned cmd, void *pvalue);
    int (*yodalite_pcm_destroy)(int fd);
    int (*yodalite_pcm_resume)(int fd);
    int (*yodalite_pcm_pause)(int fd);
    int (*yodalite_pcm_stop)(int fd);
    int (*yodalite_pcm_rerun)(int fd);
	
};

int pal_asound_init(struct asound_lapi *platform_asound_lapi);
/* Open and close a stream */
int pcm_open(unsigned int card, unsigned int device,
                     unsigned int flags, struct yodalite_pcm_config *config);
int pcm_close(int fd);
int pcm_rerun(int fd);
int pcm_pause(int fd);
int pcm_resume(int fd);
int pcm_is_ready(struct yodalite_pcm *pcm);
/* Set and get config */
int pcm_get_config(struct yodalite_pcm *pcm, struct yodalite_pcm_config *config);
int pcm_set_config(struct yodalite_pcm *pcm, struct yodalite_pcm_config *config);
/* Returns a human readable reason for the last error */
const char *pcm_get_error(struct yodalite_pcm *pcm);
/* Returns the buffer size (int frames) that should be used for pcm_write. */
unsigned int pcm_get_buffer_size(struct yodalite_pcm *pcm);
unsigned int pcm_format_to_bits(enum yodalite_pcm_format format);
unsigned int pcm_frames_to_bytes(struct yodalite_pcm *pcm, unsigned int frames);
unsigned int pcm_bytes_to_frames(struct yodalite_pcm *pcm, unsigned int bytes);
/* Returns the pcm latency in ms */
unsigned int pcm_get_latency(struct yodalite_pcm *pcm);
/* Returns available frames in pcm buffer and corresponding time stamp.
 * For an input stream, frames available are frames ready for the
 * application to read.
 * For an output stream, frames available are the number of empty frames available
 * for the application to write.
 */
int pcm_get_htimestamp(struct yodalite_pcm *pcm, unsigned int *avail,
                       struct timespec *tstamp);
/* Write data to the fifo.
 * Will start playback on the first write or on a write that
 * occurs after a fifo underrun.
 */
int pcm_write(int fd, void *data, unsigned int frame_cnt);
int pcm_read(int fd, void *data, unsigned int frame_cnt);
/*
 * mmap() support.
 */
int pcm_mmap_write(struct yodalite_pcm *pcm, const void *data, unsigned int count);
int pcm_mmap_begin(struct yodalite_pcm *pcm, void **areas, unsigned int *offset,
                   unsigned int *frames);
int pcm_mmap_commit(struct yodalite_pcm *pcm, unsigned int offset, unsigned int frames);
/* Start and stop a PCM channel that doesn't transfer data */
int pcm_start(int fd);
int pcm_stop(int fd);
/* Change avail_min after the stream has been opened with no need to stop the stream.
 * Only accepted if opened with PCM_MMAP and PCM_NOIRQ flags
 */
int pcm_set_avail_min(struct yodalite_pcm *pcm, int avail_min);
/*
 * MIXER API
 */
struct mixer;
struct mixer_ctl;
/* Open and close a mixer */
struct mixer *mixer_open(unsigned int card);
void mixer_close(struct mixer *mixer);
/* Obtain mixer controls */
unsigned int mixer_get_num_ctls(struct mixer *mixer);
struct mixer_ctl *mixer_get_ctl(struct mixer *mixer, unsigned int id);
struct mixer_ctl *mixer_get_ctl_by_name(struct mixer *mixer, const char *name);
/* Get info about mixer controls */
const char *mixer_ctl_get_name(struct mixer_ctl *ctl);
enum mixer_ctl_type mixer_ctl_get_type(struct mixer_ctl *ctl);
const char *mixer_ctl_get_type_string(struct mixer_ctl *ctl);
unsigned int mixer_ctl_get_num_values(struct mixer_ctl *ctl);
unsigned int mixer_ctl_get_num_enums(struct mixer_ctl *ctl);
const char *mixer_ctl_get_enum_string(struct mixer_ctl *ctl,
                                      unsigned int enum_id);
/* Set and get mixer controls */
int mixer_ctl_get_percent(struct mixer_ctl *ctl, unsigned int id);
int mixer_ctl_set_percent(struct mixer_ctl *ctl, unsigned int id, int percent);
int mixer_ctl_get_value(struct mixer_ctl *ctl, unsigned int id);
int mixer_ctl_set_value(struct mixer_ctl *ctl, unsigned int id, int value);
int mixer_ctl_set_enum_by_string(struct mixer_ctl *ctl, const char *string);
/* Determe range of integer mixer controls */
int mixer_ctl_get_range_min(struct mixer_ctl *ctl);
int mixer_ctl_get_range_max(struct mixer_ctl *ctl);

#if defined(__cplusplus)
}  /* extern "C" */
#endif

#endif	/*_PAL_ASOUND_H_*/
