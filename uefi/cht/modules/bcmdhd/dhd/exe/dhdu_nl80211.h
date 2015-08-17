/*
 * Definitions for DHD nl80211 driver interface.
 *
 * Copyright (C) 1999-2015, Broadcom Corporation
 * 
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 * 
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 * 
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 * $Id: $
 */

#ifndef DHDU_NL80211_H_
#define DHDU_NL80211_H_

#ifdef NL80211

#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>

/* libnl 1.x compatibility code */
#if !defined(CONFIG_LIBNL20) && !defined(CONFIG_LIBNL30)
#define nl_sock		nl_handle
#endif

struct dhd_netlink_info
{
	struct nl_sock *nl;
	struct nl_cb *cb;
	int nl_id;
	int ifidx;
};

int dhd_nl_sock_connect(struct dhd_netlink_info *dhd_nli);
void dhd_nl_sock_disconnect(struct dhd_netlink_info *dhd_nli);
int dhd_nl_do_vndr_cmd(struct dhd_netlink_info *dhd_nli, dhd_ioctl_t *ioc);

#endif /* NL80211 */

#endif /* DHDU_NL80211_H_ */
