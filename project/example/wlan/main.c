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
#include "net/wlan/wlan.h"
#include "net/wlan/wlan_defs.h"
#include "common/framework/net_ctrl.h"
#include "common/framework/platform_init.h"
#include "lwip/inet.h"
#include "common/apps/player_app.h"
#include "common/framework/platform_init.h"
#include "fs/fatfs/ff.h"
#include "../audio_play/audio.h"

#define STA_MODE_TEST		1
#define AP_MODE_TEST		1
#define MON_MODE_TEST		1


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


#if STA_MODE_TEST
char * sta_ssid = "ROKID.TC";
char * sta_psk = "rokidguys";
void sta_test(void)
{
	/* switch to sta mode */
	net_switch_mode(WLAN_MODE_STA);

	/* set ssid and password to wlan */
	wlan_sta_set((uint8_t *)sta_ssid, strlen(sta_ssid), (uint8_t *)sta_psk);
	
	/* start scan and connect to ap automatically */
	wlan_sta_enable();

	OS_Sleep(60);

	/* After one minute, disconnect from AP */
	wlan_sta_disable();
}
#endif

#if AP_MODE_TEST
char * ap_ssid = "XR872_AP";
char * ap_psk = "12345678";
void ap_test(void)
{
	/* switch to ap mode */
	net_switch_mode(WLAN_MODE_HOSTAP);

	/* disable AP to set params*/
	wlan_ap_disable();
	
	/* set ap's ssid and password */
	wlan_ap_set((uint8_t *)ap_ssid, strlen(ap_ssid), (uint8_t *)ap_psk);
	
	/* enable ap mode again */
	wlan_ap_enable();

	OS_Sleep(60);

	/* After one minute, disable AP mode */
	wlan_ap_disable();
}
#endif

#if MON_MODE_TEST
void rx_cb(uint8_t *data, uint32_t len, void *info)
{
	if (!info) {
		printf("%s(), info NULL\n", __func__);
		return;
	}

	struct ieee80211_frame *wh;
	char * str_frm_type;
	wh = (struct ieee80211_frame *)data;

	switch (wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK)
	{
	case IEEE80211_FC0_TYPE_MGT:
		switch (wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK)
		{
		case IEEE80211_FC_STYPE_ASSOC_REQ:
			str_frm_type = "association req";
			break;
		case IEEE80211_FC_STYPE_ASSOC_RESP:
			str_frm_type = "association resp";
			break;
		case IEEE80211_FC_STYPE_REASSOC_REQ:
			str_frm_type = "reassociation req";
			break;
		case IEEE80211_FC_STYPE_REASSOC_RESP:
			str_frm_type = "reassociation resp";
			break;
		case IEEE80211_FC_STYPE_PROBE_REQ:
			str_frm_type = "probe req";
			break;
		case IEEE80211_FC_STYPE_PROBE_RESP:
			str_frm_type = "probe resp";
			break;
		case IEEE80211_FC_STYPE_BEACON:
			str_frm_type = "beacon";
			break;
		case IEEE80211_FC_STYPE_ATIM:
			str_frm_type = "atim";
			break;
		case IEEE80211_FC_STYPE_DISASSOC:
			str_frm_type = "disassociation";
			break;
		case IEEE80211_FC_STYPE_AUTH:
			str_frm_type = "authentication";
			break;
		case IEEE80211_FC_STYPE_DEAUTH:
			str_frm_type = "deauthentication";
			break;
		case IEEE80211_FC_STYPE_ACTION:
			str_frm_type = "action";
			break;
		default:
			str_frm_type = "unknown mgmt";
			break;
		}
		break;
	case IEEE80211_FC0_TYPE_CTL:
		str_frm_type = "control";
		break;
	case IEEE80211_FC0_TYPE_DATA:
		str_frm_type = "data";
		break;
	default:
		str_frm_type = "unknown";
		break;
	}
	printf("recv pack type:%s\n", str_frm_type);
}

void monitor_test(void)
{
	/* register rx callback first */
	wlan_monitor_set_rx_cb(g_wlan_netif, rx_cb);

	/* switch to monitor mode */
	net_switch_mode(WLAN_MODE_MONITOR);

	OS_Sleep(60);

	/* unregister rx callback */
	wlan_monitor_set_rx_cb(g_wlan_netif, NULL);

	/* After one minute, switch back to sta mode */
	net_switch_mode(WLAN_MODE_STA);
}
#endif
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
    ret = player->play(player, "flash://0?addr=1048640&length=17626");
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

void player_audio()
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
	
	printf("player create success.\n");
	printf("you can use it to play, pause, resume, set volume and so on.\n");
	
	while (1) {
			//play_file_music();
			play_flash_music();
			//play_fifo_music();
		}
	
		OS_SemaphoreDelete(&sem);
		player_destroy(player);

}
int main(void)
{
	platform_init();
#if	STA_MODE_TEST
	sta_test();
#endif
    printf("start to platform\n");
//    player_audio();
#if AP_MODE_TEST
	ap_test();
#endif
#if MON_MODE_TEST
	monitor_test();
#endif

	return 0;
}
