/*
 * This file is part of wl1271
 *
 * Copyright (C) 1998-2009 Texas Instruments. All rights reserved.
 * Copyright (C) 2008-2009 Nokia Corporation
 *
 * Contact: Luciano Coelho <luciano.coelho@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#ifndef __EVENT_H__
#define __EVENT_H__

/*
 * Mbox events
 *
 * The event mechanism is based on a pair of event buffers (buffers A and
 * B) at fixed locations in the target's memory. The host processes one
 * buffer while the other buffer continues to collect events. If the host
 * is not processing events, an interrupt is issued to signal that a buffer
 * is ready. Once the host is done with processing events from one buffer,
 * it signals the target (with an ACK interrupt) that the event buffer is
 * free.
 */

enum {
	RSSI_SNR_TRIGGER_0_EVENT_ID              = BIT(0),
	RSSI_SNR_TRIGGER_1_EVENT_ID              = BIT(1),
	RSSI_SNR_TRIGGER_2_EVENT_ID              = BIT(2),
	RSSI_SNR_TRIGGER_3_EVENT_ID              = BIT(3),
	RSSI_SNR_TRIGGER_4_EVENT_ID              = BIT(4),
	RSSI_SNR_TRIGGER_5_EVENT_ID              = BIT(5),
	RSSI_SNR_TRIGGER_6_EVENT_ID              = BIT(6),
	RSSI_SNR_TRIGGER_7_EVENT_ID              = BIT(7),
	SCAN_COMPLETE_EVENT_ID                   = BIT(8),
	RADAR_DETECTED_EVENT_ID                  = BIT(9),
	CHANNEL_SWITCH_COMPLETE_EVENT_ID         = BIT(10),
	BSS_LOSS_EVENT_ID                        = BIT(11),
	MAX_TX_FAILURE_EVENT_ID                  = BIT(12),
	DUMMY_PACKET_EVENT_ID                    = BIT(13),
	INACTIVE_STA_EVENT_ID                    = BIT(14),
	PEER_REMOVE_COMPLETE_EVENT_ID            = BIT(15),
	PERIODIC_SCAN_COMPLETE_EVENT_ID          = BIT(16),
	BA_SESSION_RX_CONSTRAINT_EVENT_ID        = BIT(17),
	REMAIN_ON_CHANNEL_COMPLETE_EVENT_ID      = BIT(18),
	DFS_CHANNELS_CONFIG_COMPLETE_EVENT       = BIT(19),
	PERIODIC_SCAN_REPORT_EVENT_ID            = BIT(20),
	RX_BA_WIN_SIZE_CHANGE_EVENT_ID       	 = BIT(21),

	EVENT_MBOX_ALL_EVENT_ID			 = 0x7fffffff,
};

enum {
	EVENT_ENTER_POWER_SAVE_FAIL = 0,
	EVENT_ENTER_POWER_SAVE_SUCCESS,
};

#define NUM_OF_RSSI_SNR_TRIGGERS 8

struct event_mailbox {
	__le32 events_vector;

	u8 number_of_scan_results;
	u8 number_of_sched_scan_results;

	__le16 channel_switch_role_id_bitmap;

	s8 rssi_snr_trigger_metric[NUM_OF_RSSI_SNR_TRIGGERS];

	/* bitmap of removed links */
	__le32 hlid_removed_bitmap;

	/* rx ba constraint */
	__le16 rx_ba_role_id_bitmap; /* 0xfff means any role. */
	__le16 rx_ba_allowed_bitmap;

	/* bitmap of roc completed (by role id) */
	__le16 roc_completed_bitmap;

	/* bitmap of stations (by role id) with bss loss */
	__le16 bss_loss_bitmap;

	/* bitmap of stations (by HLID) which exceeded max tx retries */
	__le32 tx_retry_exceeded_bitmap;

	/* bitmap of inactive stations (by HLID) */
	__le32 inactive_sta_bitmap;

	/* rx BA win size indicated by RX_BA_WIN_SIZE_CHANGE_EVENT_ID */
	u8 rx_ba_role_id;
	u8 rx_ba_link_id;
	u8 rx_ba_win_size;
	u8 padding;

#if 0
	__le32 events_mask;
	__le32 reserved_1;
	__le32 reserved_2;

	u8 scan_tag;
	u8 completed_scan_status;
	u8 reserved_3;

	u8 soft_gemini_sense_info;
	u8 soft_gemini_protective_info;
	u8 change_auto_mode_timeout;
	u8 scheduled_scan_status;
	u8 reserved4;
	/* tuned channel (roc) */
	u8 roc_channel;

	/* discovery completed results */
	u8 discovery_tag;
	u8 number_of_preq_results;
	u8 number_of_prsp_results;
	u8 reserved_5;

	u8 reserved_6[2];

	/* Channel switch results */

	u8 reserved_7[2];

	u8 ps_poll_delivery_failure_role_ids;
	u8 stopped_role_ids;
	u8 started_role_ids;

	u8 reserved_8[9];
#endif
} __packed;

struct wl1271;

int wl1271_event_unmask(struct wl1271 *wl);
int wl1271_event_handle(struct wl1271 *wl, u8 mbox);

#endif
