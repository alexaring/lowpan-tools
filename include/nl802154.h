/*
 * nl802154.h
 *
 * Copyright (C) 2007, 2008, 2009 Siemens AG
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef NL802154_H
#define NL802154_H

#define IEEE802154_NL_NAME "802.15.4 MAC"
#define IEEE802154_MCAST_COORD_NAME "coordinator"
#define IEEE802154_MCAST_BEACON_NAME "beacon"

enum {
	__IEEE802154_ATTR_INVALID,

	IEEE802154_ATTR_DEV_NAME,
	IEEE802154_ATTR_DEV_INDEX,

	IEEE802154_ATTR_STATUS,

	IEEE802154_ATTR_SHORT_ADDR,
	IEEE802154_ATTR_HW_ADDR,
	IEEE802154_ATTR_PAN_ID,

	IEEE802154_ATTR_CHANNEL,

	IEEE802154_ATTR_COORD_SHORT_ADDR,
	IEEE802154_ATTR_COORD_HW_ADDR,
	IEEE802154_ATTR_COORD_PAN_ID,

	IEEE802154_ATTR_SRC_SHORT_ADDR,
	IEEE802154_ATTR_SRC_HW_ADDR,
	IEEE802154_ATTR_SRC_PAN_ID,

	IEEE802154_ATTR_DEST_SHORT_ADDR,
	IEEE802154_ATTR_DEST_HW_ADDR,
	IEEE802154_ATTR_DEST_PAN_ID,

	IEEE802154_ATTR_CAPABILITY,
	IEEE802154_ATTR_REASON,
	IEEE802154_ATTR_SCAN_TYPE,
	IEEE802154_ATTR_CHANNELS,
	IEEE802154_ATTR_DURATION,
	IEEE802154_ATTR_ED_LIST,
	IEEE802154_ATTR_BCN_ORD,
	IEEE802154_ATTR_SF_ORD,
	IEEE802154_ATTR_PAN_COORD,
	IEEE802154_ATTR_BAT_EXT,
	IEEE802154_ATTR_COORD_REALIGN,
	IEEE802154_ATTR_SEC,

	IEEE802154_ATTR_PAGE,
	IEEE802154_ATTR_CHANNEL_PAGE_LIST,

	__IEEE802154_ATTR_MAX,
};

#define IEEE802154_ATTR_MAX (__IEEE802154_ATTR_MAX - 1)

extern struct nla_policy ieee802154_policy[];

/* commands */
/* REQ should be responded with CONF
 * and INDIC with RESP
 */
enum {
	__IEEE802154_COMMAND_INVALID,

	IEEE802154_ASSOCIATE_REQ,
	IEEE802154_ASSOCIATE_CONF,
	IEEE802154_DISASSOCIATE_REQ,
	IEEE802154_DISASSOCIATE_CONF,
	IEEE802154_GET_REQ,
	IEEE802154_GET_CONF,
	IEEE802154_RESET_REQ,
	IEEE802154_RESET_CONF,
	IEEE802154_SCAN_REQ,
	IEEE802154_SCAN_CONF,
	IEEE802154_SET_REQ,
	IEEE802154_SET_CONF,
	IEEE802154_START_REQ,
	IEEE802154_START_CONF,
	IEEE802154_SYNC_REQ,
	IEEE802154_POLL_REQ,
	IEEE802154_POLL_CONF,

	IEEE802154_ASSOCIATE_INDIC,
	IEEE802154_ASSOCIATE_RESP,
	IEEE802154_DISASSOCIATE_INDIC,
	IEEE802154_BEACON_NOTIFY_INDIC,
	IEEE802154_ORPHAN_INDIC,
	IEEE802154_ORPHAN_RESP,
	IEEE802154_COMM_STATUS_INDIC,
	IEEE802154_SYNC_LOSS_INDIC,

	IEEE802154_GTS_REQ, /* Not supported yet */
	IEEE802154_GTS_INDIC, /* Not supported yet */
	IEEE802154_GTS_CONF, /* Not supported yet */
	IEEE802154_RX_ENABLE_REQ, /* Not supported yet */
	IEEE802154_RX_ENABLE_CONF, /* Not supported yet */

	IEEE802154_LIST_IFACE,
	IEEE802154_LIST_PHY,

	__IEEE802154_CMD_MAX,
};

#define IEEE802154_CMD_MAX (__IEEE802154_CMD_MAX - 1)

#endif
