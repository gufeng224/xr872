/*
 * (C) Copyright 2019 Rokid Corp.
 * Zhu Bin <bin.zhu@rokid.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include <platform.h>
#include <pal_asound.h>

#ifdef CONFIG_MEM_EXT_ENABLE
#define yodalite_malloc malloc_ext
#define yodalite_free free_ext
#else
#define yodalite_malloc malloc
#define yodalite_free free
#endif
#define ASND_LOG printf
static struct asound_lapi pal_asound_lapi = {0};

int pal_asound_init(struct asound_lapi *platform_asound_lapi)
{
    if(!platform_asound_lapi)
        return -STATUS_EINVAL;
    pal_asound_lapi.yodalite_pcm_init = platform_asound_lapi->yodalite_pcm_init;
    pal_asound_lapi.yodalite_pcm_read = platform_asound_lapi->yodalite_pcm_read;
    pal_asound_lapi.yodalite_pcm_write = platform_asound_lapi->yodalite_pcm_write;
    pal_asound_lapi.yodalite_pcm_ioctl = platform_asound_lapi->yodalite_pcm_ioctl;
    pal_asound_lapi.yodalite_pcm_destroy = platform_asound_lapi->yodalite_pcm_destroy;
    pal_asound_lapi.yodalite_pcm_resume = platform_asound_lapi->yodalite_pcm_resume;
    pal_asound_lapi.yodalite_pcm_pause = platform_asound_lapi->yodalite_pcm_pause;
    pal_asound_lapi.yodalite_pcm_rerun = platform_asound_lapi->yodalite_pcm_rerun;
    pal_asound_lapi.yodalite_pcm_stop = platform_asound_lapi->yodalite_pcm_stop;

    return STATUS_OK;
}

unsigned int yodalite_pcm_format_to_bits(enum yodalite_pcm_format format)
{
    switch (format) {
    case YODA_PCM_FORMAT_S32_LE:
        return 32;
    default:
    case YODA_PCM_FORMAT_S16_LE:
        return 16;
    };
}
unsigned int yodalite_pcm_bytes_to_frames(struct yodalite_pcm *pcm, unsigned int bytes)
{
    return bytes / (pcm->config.channels *
        (yodalite_pcm_format_to_bits(pcm->config.format) >> 3));
}
unsigned int yodalite_pcm_frames_to_bytes(struct yodalite_pcm *pcm, unsigned int frames)
{
    return frames * pcm->config.channels *
        (yodalite_pcm_format_to_bits(pcm->config.format) >> 3);
}


/** Opens a PCM.
 * @param card The card that the pcm belongs to.
 *  The default card is zero.
 * @param device The device that the pcm belongs to.
 *  The default device is zero.
 * @param flags Specify characteristics and functionality about the pcm.
 *  May be a bitwise AND of the following:
 *   - @ref PCM_IN
 *   - @ref PCM_OUT
 *   - @ref PCM_MMAP
 *   - @ref PCM_NOIRQ
 *   - @ref PCM_MONOTONIC
 * @param config The hardware and software parameters to open the PCM with.
 * @returns A PCM structure.
 *  If an error occurs allocating memory for the PCM, NULL is returned.
 *  Otherwise, client code should check that the PCM opened properly by calling @ref pcm_is_ready.
 *  If @ref pcm_is_ready, check @ref pcm_get_error for more information.
 * @ingroup libtinyalsa-pcm
 */
int pcm_open(unsigned int card, unsigned int device,
                     unsigned int flags, struct yodalite_pcm_config *config)
{
    struct asound_resource asound_res;
    int fd;
#if 0
    struct pcm *pcm;
    char fn[256];
    int rc;
    

    pcm = (struct pcm*)yodalite_malloc(sizeof(struct pcm));
    if (!pcm || !config)
        return NULL; /* TODO: could support default config here */
    pcm->config = *config;
    pcm->flags = flags;
    snprintf(fn, sizeof(fn), "%s%uD%u%c", PCM_PATH, card, device, flags & PCM_IN ? 'c' : 'p');
    pcm->fd = platform_device_open(fn, DEV_RDWR);
    if (pcm->fd <= 0) {
        ASND_LOG("cannot open device '%s'", fn);
        yodalite_free(pcm);
        return NULL;
    }
    if (platform_device_ioctl(pcm->fd, SNDRV_PCM_CONFIG_IOCTL, config)) {
        ASND_LOG("%s cannot set pcm_config", fn);
        goto fail_close;
    }
#endif
    asound_res.asound_id = card;
    asound_res.channels = config->channels;
    asound_res.sample_rate = config->rate;
    asound_res.format = config->format;
    asound_res.pcm_direction = (flags & YODALITE_PCM_IN) ? YODALITE_PCM_IN: YODALITE_PCM_OUT;
    asound_res.source = config->source;// add by gufeng
    asound_res.file_source = config->file_source;
    asound_res.url_str = config->url_str;
    yodalite_free(config);
    if(pal_asound_lapi.yodalite_pcm_init)
        fd = pal_asound_lapi.yodalite_pcm_init(&asound_res);
    else {
	fd = -1;
        ASND_LOG("cannot get pcm fd");
        return fd;
    }
#if 0
    if (flags & PCM_NOIRQ) {
        if (!(flags & PCM_MMAP)) {
            ASND_LOG("noirq only currently supported with mmap().");
            goto fail;
        }
        pcm->noirq_frames_per_msec = config->rate / 1000;
    }
    pcm->buffer_size = config->period_count * config->period_size;
    if (!config->start_threshold) {
        if (pcm->flags & PCM_IN)
            pcm->config.start_threshold = 1;
        else
            pcm->config.start_threshold = config->period_count * config->period_size / 2;
    }
    /* pick a high stop threshold - todo: does this need further tuning */
    if (!config->stop_threshold) {
        if (pcm->flags & PCM_IN)
            pcm->config.stop_threshold = config->period_count * config->period_size * 10;
        else
            pcm->config.stop_threshold = config->period_count * config->period_size;
    }
    if (!pcm->config.avail_min) {
        if (pcm->flags & PCM_MMAP)
            pcm->config.avail_min = pcm->config.period_size;
        else
            pcm->config.avail_min = 1;
    }
    pcm->boundary = pcm->buffer_size;
   /* while (pcm->boundary * 2 <= INT_MAX - pcm->buffer_size)
        pcm->boundary *= 2;
    pcm->underruns = 0;*/
#endif
    return fd;
/*
fail:
fail_close:
    if(pal_asound_lapi.yodalite_pcm_destroy)
            pal_asound_lapi.yodalite_pcm_destroy(pcm->fd);
    fd = -1;
    //yodalite_free(pcm);
    return NULL;*/
}

/** Checks if a PCM file has been opened without error.
* @param pcm A PCM handle.
*  May be NULL.
* @return If a PCM's file descriptor is not valid or the pointer is NULL, it returns zero.
*  Otherwise, the function returns one.
* @ingroup libtinyalsa-pcm
*/
int pcm_is_ready(struct yodalite_pcm *pcm)
{
    if (pcm != NULL) {
        return pcm->fd >= 0;
    }
    return 0;
}

/** Writes audio samples to PCM.
 * If the PCM has not been started, it is started in this function.
 * This function is only valid for PCMs opened with the @ref PCM_OUT flag.
 * This function is not valid for PCMs opened with the @ref PCM_MMAP flag.
 * @param pcm A PCM handle.
 * @param data The audio sample array
 * @param frame_cnt The number of pcm frame occupied by the sample array.
 * @return On success, this function returns zero; otherwise, a negative number.
 * @deprecated
 * @ingroup libtinyalsa-pcm
 */
int pcm_write(int fd, void *data, unsigned int frame_cnt)
{
    int ret = -STATUS_EIO;
    if(pal_asound_lapi.yodalite_pcm_write)
        ret = pal_asound_lapi.yodalite_pcm_write(fd, data, frame_cnt);
    return ret;
}

/** Reads audio samples from PCM.
 * If the PCM has not been started, it is started in this function.
 * This function is only valid for PCMs opened with the @ref PCM_IN flag.
 * This function is not valid for PCMs opened with the @ref PCM_MMAP flag.
 * @param pcm A PCM handle.
 * @param data The audio sample array
 * @param frame_cnt The number of pcm frame occupied by the sample array.
 * @return On success, this function returns zero; otherwise, a negative number.
 * @deprecated
 * @ingroup libtinyalsa-pcm
 */
int pcm_read(int fd, void *data, unsigned int frame_cnt)
{
    int ret = -STATUS_EIO;
    if(pal_asound_lapi.yodalite_pcm_read)
        ret = pal_asound_lapi.yodalite_pcm_read(fd, data, frame_cnt);
    return ret;
}

/** Closes a PCM returned by @ref pcm_open.
 * @param pcm A PCM returned by @ref pcm_open.
 *  May not be NULL.
 * @return Always returns zero.
 * @ingroup libtinyalsa-pcm
 */
int pcm_close(int fd)
{
   int ret = -STATUS_EIO;
   if(pal_asound_lapi.yodalite_pcm_destroy)
      ret = pal_asound_lapi.yodalite_pcm_destroy(fd);
   return ret;
}

int pcm_resume(int fd)
{
    int ret = -STATUS_EIO;
    if(pal_asound_lapi.yodalite_pcm_resume)
       ret = pal_asound_lapi.yodalite_pcm_resume(fd); //pcm_bytes_to_frames(pcm, count));
    return ret;
}

int pcm_pause(int fd)
{
    int ret = -STATUS_EIO;
    if(pal_asound_lapi.yodalite_pcm_pause)
        ret = pal_asound_lapi.yodalite_pcm_pause(fd); //pcm_bytes_to_frames(pcm, count));
    return ret;
}


int pcm_stop(int fd)
{
    int ret = -STATUS_EIO;
    if(pal_asound_lapi.yodalite_pcm_pause)
        ret = pal_asound_lapi.yodalite_pcm_stop(fd); //pcm_bytes_to_frames(pcm, count));
    return ret;
}


int pcm_rerun(int fd)
{
    int ret = -STATUS_EIO;
    if(pal_asound_lapi.yodalite_pcm_pause)
        ret = pal_asound_lapi.yodalite_pcm_rerun(fd); //pcm_bytes_to_frames(pcm, count));
    return ret;
}

