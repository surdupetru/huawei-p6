/*
 * This file is part of wl1271
 *
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

#include "wlcore.h"
#include "debug.h"
#include "io.h"
#include "event.h"
#include "ps.h"
#include "scan.h"
#include "wl12xx_80211.h"

static void wl1271_event_rssi_trigger(struct wl1271 *wl,
				      struct wl12xx_vif *wlvif,
				      struct event_mailbox *mbox)
{
	struct ieee80211_vif *vif = wl12xx_wlvif_to_vif(wlvif);
	enum nl80211_cqm_rssi_threshold_event event;
	s8 metric = mbox->rssi_snr_trigger_metric[0];

	wl1271_debug(DEBUG_EVENT, "RSSI trigger metric: %d", metric);

	if (metric <= wlvif->rssi_thold)
		event = NL80211_CQM_RSSI_THRESHOLD_EVENT_LOW;
	else
		event = NL80211_CQM_RSSI_THRESHOLD_EVENT_HIGH;

	if (event != wlvif->last_rssi_event)
		ieee80211_cqm_rssi_notify(vif, event, GFP_KERNEL);
	wlvif->last_rssi_event = event;
}

static void wl1271_stop_ba_event(struct wl1271 *wl, struct wl12xx_vif *wlvif)
{
	struct ieee80211_vif *vif = wl12xx_wlvif_to_vif(wlvif);

	if (wlvif->bss_type != BSS_TYPE_AP_BSS) {
		u8 hlid = wlvif->sta.hlid;
		if (!wl->links[hlid].ba_bitmap)
			return;
		ieee80211_stop_rx_ba_session(vif, wl->links[hlid].ba_bitmap,
					     vif->bss_conf.bssid);
	} else {
		u8 hlid;
		struct wl1271_link *lnk;
		for_each_set_bit(hlid, wlvif->ap.sta_hlid_map,
				 WL12XX_MAX_LINKS) {
			lnk = &wl->links[hlid];
			if (!lnk->ba_bitmap)
				continue;

			ieee80211_stop_rx_ba_session(vif,
						     lnk->ba_bitmap,
						     lnk->addr);
		}
	}
}

static void wl1271_event_mbox_dump(struct event_mailbox *mbox)
{
	wl1271_debug(DEBUG_EVENT, "MBOX DUMP:");
	wl1271_debug(DEBUG_EVENT, "\tvector: 0x%x", mbox->events_vector);
}

static int wl1271_event_process(struct wl1271 *wl)
{
	struct event_mailbox *mbox = wl->mbox;
	struct ieee80211_vif *vif;
	struct wl12xx_vif *wlvif;
	u32 vector;
	bool disconnect_sta = false;
	unsigned long sta_bitmap = 0;
	int ret;

	wl1271_event_mbox_dump(mbox);

	vector = le32_to_cpu(mbox->events_vector);
	wl1271_debug(DEBUG_EVENT, "vector: 0x%x", vector);

	if (vector & SCAN_COMPLETE_EVENT_ID) {
		wl1271_debug(DEBUG_EVENT, "scan results: %d",
			     mbox->number_of_scan_results);

		wl12xx_scan_completed(wl);
	}

	if (vector & PERIODIC_SCAN_REPORT_EVENT_ID) {
		wl1271_debug(DEBUG_EVENT,
			     "PERIODIC_SCAN_REPORT_EVENT (results %d)",
			     mbox->number_of_sched_scan_results);

		wl1271_scan_sched_scan_results(wl);
	}

	if (vector & PERIODIC_SCAN_COMPLETE_EVENT_ID) {
		wl1271_debug(DEBUG_EVENT,
			     "PERIODIC_SCAN_COMPLETE_EVENT (results 0x%0x)",
			     mbox->number_of_sched_scan_results);
		if (wl->sched_vif) {
			ieee80211_sched_scan_stopped(wl->hw);
			wl->sched_vif = NULL;
		}
	}

	/*
	 * We are HW_MONITOR device. On beacon loss - queue
	 * connection loss work. Cancel it on REGAINED event.
	 */
	if (vector & BSS_LOSS_EVENT_ID) {
		int delay = wl->conf.conn.synch_fail_thold *
					wl->conf.conn.bss_lose_timeout;
		unsigned long roles_bitmap =
			le16_to_cpu(mbox->bss_loss_bitmap);

		wl1271_info("Beacon loss detected. roles:0x%lx", roles_bitmap);

		wl12xx_for_each_wlvif_sta(wl, wlvif) {
			if (wlvif->role_id == WL12XX_INVALID_ROLE_ID ||
			    !test_bit(wlvif->role_id , &roles_bitmap))
				continue;

			vif = wl12xx_wlvif_to_vif(wlvif);

			/* don't attempt roaming in case of p2p */
			if (wlvif->p2p) {
				ieee80211_connection_loss(vif);
				continue;
			}

			/*
			 * if the work is already queued, it should take place.
			 * We don't want to delay the connection loss
			 * indication any more.
			 */
			ieee80211_queue_delayed_work(wl->hw,
						&wlvif->connection_loss_work,
						msecs_to_jiffies(delay));

			ieee80211_cqm_rssi_notify(
					vif,
					NL80211_CQM_RSSI_BEACON_LOSS_EVENT,
					GFP_KERNEL);
		}
	}

	if (vector & RSSI_SNR_TRIGGER_0_EVENT_ID) {
		/* TODO: check actual multi-role support */
		wl1271_debug(DEBUG_EVENT, "RSSI_SNR_TRIGGER_0_EVENT");
		wl12xx_for_each_wlvif_sta(wl, wlvif) {
			wl1271_event_rssi_trigger(wl, wlvif, mbox);
		}
	}

	if (vector & BA_SESSION_RX_CONSTRAINT_EVENT_ID) {
		unsigned long roles_bitmap =
			le16_to_cpu(mbox->rx_ba_role_id_bitmap);
		unsigned long allowed_bitmap =
			le16_to_cpu(mbox->rx_ba_allowed_bitmap);

		wl1271_debug(DEBUG_EVENT, "BA_SESSION_RX_CONSTRAINT_EVENT_ID. "
			     "ba_allowed = 0x%lx, role_id=0x%lx",
			     allowed_bitmap, roles_bitmap);

		wl12xx_for_each_wlvif(wl, wlvif) {
			if (wlvif->role_id == WL12XX_INVALID_ROLE_ID ||
			    !test_bit(wlvif->role_id , &roles_bitmap))
				continue;

			wlvif->ba_allowed = !!test_bit(wlvif->role_id,
						       &allowed_bitmap);
			if (!wlvif->ba_allowed)
				wl1271_stop_ba_event(wl, wlvif);
		}
	}

	if (vector & RX_BA_WIN_SIZE_CHANGE_EVENT_ID) {
		u8 role_id = mbox->rx_ba_role_id;
		u8 link_id = mbox->rx_ba_link_id;
		u8 win_size = mbox->rx_ba_win_size;
		int prev_win_size;

		wl1271_debug(DEBUG_EVENT,
			     "%s. role_id=%u link_id=%u win_size=%u",
			     "RX_BA_WIN_SIZE_CHANGE_EVENT_ID",
			     role_id, link_id, win_size);

		wlvif = wl->links[link_id].wlvif;
		if (unlikely(!wlvif)) {
			wl1271_error("%s. link_id wlvif is null",
				     "RX_BA_WIN_SIZE_CHANGE_EVENT_ID");

			goto out_event;
		}

		if (unlikely(wlvif->role_id != role_id)) {
			wl1271_error("%s. wlvif has different role_id=%d",
				     "RX_BA_WIN_SIZE_CHANGE_EVENT_ID",
				     wlvif->role_id);

			goto out_event;
		}

		prev_win_size = wlcore_rx_ba_max_subframes(wl, link_id);
		if (unlikely(prev_win_size < 0)) {
			wl1271_error("%s. cannot get link rx_ba_max_subframes",
				     "RX_BA_WIN_SIZE_CHANGE_EVENT_ID");

			goto out_event;
		}

		if ((u8) prev_win_size <= win_size) {
			/* This not supposed to happen unless a FW bug */
			wl1271_error("%s. prev_win_size(%d) <= win_size(%d)",
				       "RX_BA_WIN_SIZE_CHANGE_EVENT_ID",
					prev_win_size, win_size);

			goto out_event;
		}

		vif = wl12xx_wlvif_to_vif(wlvif);
		if (unlikely(!vif)) {
			wl1271_error("%s. link_id vif is null",
				     "RX_BA_WIN_SIZE_CHANGE_EVENT_ID");

			goto out_event;
		}

		/*
		 * Call MAC routine to update win_size and stop all link active
		 * BA sessions. This routine returns 0 on failure or previous
		 * win_size on success
		 */
		ieee80211_change_rx_ba_max_subframes(vif,
					(wlvif->bss_type != BSS_TYPE_AP_BSS ?
						vif->bss_conf.bssid :
						wl->links[link_id].addr),
					win_size);
	}

out_event:

	if (vector & CHANNEL_SWITCH_COMPLETE_EVENT_ID) {
		unsigned long roles_bitmap =
			le16_to_cpu(mbox->channel_switch_role_id_bitmap);

		wl1271_debug(DEBUG_EVENT, "CHANNEL_SWITCH_COMPLETE_EVENT_ID. roles=0x%lx",
			     roles_bitmap);

		/*
		 * we get this event only when channel switch was completed
		 * successfully. there is no indication for failure...
		 */
		wl12xx_for_each_wlvif_sta(wl, wlvif) {
			if (wlvif->role_id == WL12XX_INVALID_ROLE_ID ||
			    !test_bit(wlvif->role_id , &roles_bitmap))
				continue;

			if (!test_and_clear_bit(WLVIF_FLAG_CS_PROGRESS,
						&wlvif->flags))
				continue;

			vif = wl12xx_wlvif_to_vif(wlvif);

			ieee80211_chswitch_done(vif, true);
			cancel_delayed_work(&wlvif->channel_switch_work);
		}
	}

	if ((vector & DUMMY_PACKET_EVENT_ID)) {
		wl1271_debug(DEBUG_EVENT, "DUMMY_PACKET_ID_EVENT_ID");
		ret = wl1271_tx_dummy_packet(wl);
		if (ret < 0)
			return ret;
	}

	/*
	 * "TX retries exceeded" has a different meaning according to mode.
	 * In AP mode the offending station is disconnected.
	 */
	if (vector & MAX_TX_FAILURE_EVENT_ID) {
		wl1271_debug(DEBUG_EVENT, "MAX_TX_FAILURE_EVENT_ID");
		sta_bitmap |= le32_to_cpu(mbox->tx_retry_exceeded_bitmap);
		disconnect_sta = true;
	}

	if (vector & INACTIVE_STA_EVENT_ID) {
		wl1271_debug(DEBUG_EVENT, "INACTIVE_STA_EVENT_ID");
		sta_bitmap |= le32_to_cpu(mbox->inactive_sta_bitmap);
		disconnect_sta = true;
	}

	if (vector & REMAIN_ON_CHANNEL_COMPLETE_EVENT_ID) {
		wl1271_debug(DEBUG_EVENT,
			     "REMAIN_ON_CHANNEL_COMPLETE_EVENT_ID");
		ieee80211_ready_on_channel(wl->hw);
	}

	if (disconnect_sta) {
		u32 num_packets = wl->conf.tx.max_tx_retries;
		struct ieee80211_sta *sta;
		const u8 *addr;
		int h;

		for_each_set_bit(h, &sta_bitmap, WL12XX_MAX_LINKS) {
			bool found = false;
			/* find the ap vif connected to this sta */
			wl12xx_for_each_wlvif_ap(wl, wlvif) {
				if (!test_bit(h, wlvif->ap.sta_hlid_map))
					continue;
				found = true;
				break;
			}
			if (!found)
				continue;

			vif = wl12xx_wlvif_to_vif(wlvif);
			addr = wl->links[h].addr;

			rcu_read_lock();
			sta = ieee80211_find_sta(vif, addr);
			if (sta) {
				wl1271_debug(DEBUG_EVENT, "remove sta %d", h);
				ieee80211_report_low_ack(sta, num_packets);
			}
			rcu_read_unlock();
		}
	}
	return 0;
}

int wl1271_event_unmask(struct wl1271 *wl)
{
	int ret;

	ret = wl1271_acx_event_mbox_mask(wl, ~(wl->event_mask));
	if (ret < 0)
		return ret;

	return 0;
}

int wl1271_event_handle(struct wl1271 *wl, u8 mbox_num)
{
	int ret;

	wl1271_debug(DEBUG_EVENT, "EVENT on mbox %d", mbox_num);

	if (mbox_num > 1)
		return -EINVAL;

	/* first we read the mbox descriptor */
	ret = wlcore_read(wl, wl->mbox_ptr[mbox_num], wl->mbox,
			  sizeof(*wl->mbox), false);
	if (ret < 0)
		return ret;

	/* process the descriptor */
	ret = wl1271_event_process(wl);
	if (ret < 0)
		return ret;

	/*
	 * TODO: we just need this because one bit is in a different
	 * place.  Is there any better way?
	 */
	ret = wl->ops->ack_event(wl);

	return ret;
}
