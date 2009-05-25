/*
 * Linux IEEE 802.15.4 userspace tools
 *
 * Copyright (C) 2008, 2009 Siemens AG
 *
 * Written-by: Dmitry Eremin-Solenikov
 * Written-by: Sergey Lapin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <sys/types.h>
#include <unistd.h>
#include <grp.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <errno.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include <ieee802154.h>
#define IEEE80215_NL_WANT_POLICY
#include <ieee80215-nl.h>
#include <libcommon.h>
#include <signal.h>
#include <getopt.h>
#include <libgen.h>

#include <logging.h>

#include "addrdb.h"

#define u64 uint64_t


#define PID_BUF_LEN 32

static int seq_expected;
static int family;
static struct nl_handle *nl;
static const char *iface;
static char lease_file[PATH_MAX];

extern int yydebug;

static void log_msg_nl_perror(char *s)
{
	if (nl_get_errno())
		log_msg(0, "%s: %s", s, nl_geterror());
}

int mlme_start(uint16_t short_addr, uint16_t pan, uint8_t is_coordinator, const char * iface)
{
	struct nl_msg *msg = nlmsg_alloc();
	log_msg(0, "mlme_start\n");
	genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, family, 0, NLM_F_REQUEST, IEEE80215_START_REQ, /* vers */ 1);
	nla_put_string(msg, IEEE80215_ATTR_DEV_NAME, iface);
	nla_put_u16(msg, IEEE80215_ATTR_COORD_PAN_ID, pan);
	nla_put_u16(msg, IEEE80215_ATTR_COORD_SHORT_ADDR, short_addr);
#if 0
	nla_put_u8(msg, IEEE80215_ATTR_CHANNEL, channel);
	nla_put_u8(msg, IEEE80215_ATTR_BCN_ORD, bcn_ord);
	nla_put_u8(msg, IEEE80215_ATTR_SF_ORD, sf_ord);
#endif
	nla_put_u8(msg, IEEE80215_ATTR_PAN_COORD, is_coordinator);
#if 0
	nla_put_u8(msg, IEEE80215_ATTR_BAT_EXT, battery_ext);
	nla_put_u8(msg, IEEE80215_ATTR_COORD_REALIGN, coord_realign);
#endif
	nl_send_auto_complete(nl, msg);
	log_msg_nl_perror("nl_send_auto_complete");
	return 0;
}

static int coordinator_associate(struct genlmsghdr *ghdr, struct nlattr **attrs)
{
	log_msg(0, "Associate requested\n");

	if (!attrs[IEEE80215_ATTR_DEV_INDEX] ||
	    !attrs[IEEE80215_ATTR_SRC_HW_ADDR] ||
	    !attrs[IEEE80215_ATTR_CAPABILITY])
		return -EINVAL;

	// FIXME: checks!!!

	struct nl_msg *msg = nlmsg_alloc();
	uint8_t cap = nla_get_u8(attrs[IEEE80215_ATTR_CAPABILITY]);
	uint16_t shaddr = 0xfffe;

	genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, family, 0, NLM_F_REQUEST, IEEE80215_ASSOCIATE_RESP, /* vers */ 1);

	if (cap & (1 << 7)) { /* FIXME: constant */
		uint8_t hwa[IEEE80215_ADDR_LEN];
		NLA_GET_HW_ADDR(attrs[IEEE80215_ATTR_SRC_HW_ADDR], hwa);
		shaddr = addrdb_alloc(hwa);
		addrdb_dump_leases(lease_file);
	}

	nla_put_u32(msg, IEEE80215_ATTR_DEV_INDEX, nla_get_u32(attrs[IEEE80215_ATTR_DEV_INDEX]));
	nla_put_u32(msg, IEEE80215_ATTR_STATUS, (shaddr != 0xffff) ? 0x0: 0x01);
	nla_put_u64(msg, IEEE80215_ATTR_DEST_HW_ADDR, nla_get_u64(attrs[IEEE80215_ATTR_SRC_HW_ADDR]));
	nla_put_u16(msg, IEEE80215_ATTR_DEST_SHORT_ADDR, shaddr);

	nl_send_auto_complete(nl, msg);

	log_msg_nl_perror("nl_send_auto_complete");

	return 0;
}

static int coordinator_disassociate(struct genlmsghdr *ghdr, struct nlattr **attrs)
{
	log_msg(0, "Disassociate requested\n");

	if (!attrs[IEEE80215_ATTR_DEV_INDEX] ||
	    !attrs[IEEE80215_ATTR_REASON] ||
	    (!attrs[IEEE80215_ATTR_SRC_HW_ADDR] && !attrs[IEEE80215_ATTR_SRC_SHORT_ADDR]))
		return -EINVAL;

	// FIXME: checks!!!
	if (attrs[IEEE80215_ATTR_SRC_HW_ADDR]) {
		uint8_t hwa[IEEE80215_ADDR_LEN];
		NLA_GET_HW_ADDR(attrs[IEEE80215_ATTR_SRC_HW_ADDR], hwa);
		addrdb_free_hw(hwa);
	} else {
		uint16_t short_addr = nla_get_u16(attrs[IEEE80215_ATTR_SRC_SHORT_ADDR]);
		addrdb_free_short(short_addr);
	}
	addrdb_dump_leases(lease_file);

	return 0;
}

static int parse_cb(struct nl_msg *msg, void *arg)
{
	struct nlmsghdr *nlh = nlmsg_hdr(msg);
	struct nlattr *attrs[IEEE80215_ATTR_MAX+1];
        struct genlmsghdr *ghdr;
	const char *name;

	// Validate message and parse attributes
	genlmsg_parse(nlh, 0, attrs, IEEE80215_ATTR_MAX, ieee80215_policy);

        ghdr = nlmsg_data(nlh);

	if (!attrs[IEEE80215_ATTR_DEV_NAME])
		return -EINVAL;

	name = nla_get_string(attrs[IEEE80215_ATTR_DEV_NAME]);
	log_msg(0, "Received command %d (%d) for interface %s\n", ghdr->cmd, ghdr->version, name);

	name = nla_get_string(attrs[IEEE80215_ATTR_DEV_NAME]);
	if (strcmp(name, iface)) {
		return 0;
	}

	switch (ghdr->cmd) {
		case IEEE80215_ASSOCIATE_INDIC:
			return coordinator_associate(ghdr, attrs);
		case IEEE80215_DISASSOCIATE_INDIC:
			return coordinator_disassociate(ghdr, attrs);
	}

	if (!attrs[IEEE80215_ATTR_HW_ADDR])
		return -EINVAL;

	uint64_t addr = nla_get_u64(attrs[IEEE80215_ATTR_HW_ADDR]);
	uint8_t buf[8];
	memcpy(buf, &addr, 8);

	log_msg(0, "Addr for %s is %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
			nla_get_string(attrs[IEEE80215_ATTR_DEV_NAME]),
			buf[0], buf[1],	buf[2], buf[3],
			buf[4], buf[5],	buf[6], buf[7]);

	return 0;
}

static int seq_check(struct nl_msg *msg, void *arg) {
	if (nlmsg_get_src(msg)->nl_groups)
		return NL_OK;

	uint32_t seq = nlmsg_hdr(msg)->nlmsg_seq;

	if (seq == seq_expected) {
		seq_expected ++;
		return NL_OK;
	}

	log_msg(0, "Sequence number mismatch: %x != %x\n", seq, seq_expected);

	return NL_SKIP;
}

void dump_lease_handler(int t)
{
	addrdb_dump_leases(lease_file);
}

int die_flag = 0;

void cleanup(int ret)
{
	if(ret == 0)
		addrdb_dump_leases(lease_file);
	nl_close(nl);
	unlink(PID_FILE);
	exit(ret);	
}

void exit_handler(int t)
{
	die_flag = 1;
	cleanup(0);
}

void usage(char * name)
{
	printf("Usage: %s [OPTION]... -i IFACE\n", name);
	printf("Provide a userspace part of IEEE 802.15.4 coordinator on specified IFACE.\n\n");
	printf(	" -l lease_file      Where we store lease file.\n"
		" -d debug_level     Set debug level of application.\n"
		"                    Will not demonize on levels > 0.\n"
		" -m range_min       Minimal new 16-bit address allocated.\n"
		" -n range_max       Maximal new 16-bit address allocated.\n"
		" -i iface           Interface to work with.\n"
		" -s addr            16-bit address of coordinator (hexadecimal).\n"
		" -p addr            16-bit address of PAN (hexadecimal).\n"
		" -h, --help         This usage information.\n"
		" -v, --version      Print version information.\n"
	);
}
#ifdef HAVE_GETOPT_LONG
static struct option long_options[] = {
	{"help", 0, 0, 0},
	{"version", 0, 0, 1},
	{0, 0, 0, -1}
};
#endif

int range_min, range_max;

int main(int argc, char **argv)
{
	struct sigaction sa;
	int opt, debug, pid_fd, uid;
	uint16_t pan = 0, short_addr = 0;
	char pname[PATH_MAX];
	char * p;

	debug = 0;
	range_min = 0x8000;
	range_max = 0xfffd;

	memcpy(lease_file, LEASE_FILE, sizeof(lease_file));

	p = getenv("LEASE_FILE");
	if(p)
		strncpy(lease_file, p, PATH_MAX);

	strncpy(pname, argv[0], PATH_MAX);

#ifndef HAVE_GETOPT_LONG
	while ((opt = getopt(argc, argv, "l:d:m:n:i:s:p:hv")) != -1) {
#else
	while(1) {
		int option_index = 0;
		opt = getopt_long(argc, argv, "l:d:m:n:i:s:p:hv",
				long_options, &option_index);
		fprintf(stderr, "Opt: %c (%hhx)\n", opt, opt);
		if (opt == -1)
			break;
#endif
		fprintf(stderr, "Opt: %c (%hhx)\n", opt, opt);
		switch(opt) {
		case 'l':
			strncpy(lease_file, optarg, PATH_MAX);
			break;
		case 'd':
			debug = atoi(optarg);
			break;
		case 'm':
			range_min = atoi(optarg);
			break;
		case 'n':
			range_max = atoi(optarg);
			break;
		case 'i':
			iface = strdup(optarg);
			break;
		case 'p': /* PAN address */
			pan = strtol(optarg, NULL, 16);
			break;
		case 's': /* 16-bit address */
			short_addr = strtol(optarg, NULL, 16);
			break;
		case 1:
		case 'v':
			printf("izcoordinator %s\nCopyright (C) 2008, 2009 by authors team\n", VERSION);
			return 0;
		case 0:
		case 'h':
			usage(pname);
			return 0;
		default:
			usage(pname);
			return -1;
		}
	}
	if (pan == 0 || short_addr == 0) {
		fprintf(stderr, "PAN address and/or 16-bit address were not set\n");
		usage(pname);
		return -1;
	}
	lease_file[sizeof(lease_file)-1] = '\0';
	if (debug > 1)
		yydebug = 1; /* Parser debug */
	else
		yydebug = 0;

	init_log(basename(argv[0]), debug);

	if (!iface) {
		usage(pname);
		return -1;
	}

	addrdb_init(range_min, range_max);
	addrdb_parse(lease_file);

	sa.sa_handler = dump_lease_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGUSR1, &sa, NULL);
	sa.sa_handler = dump_lease_handler;
	sigaction(SIGHUP, &sa, NULL);

	sa.sa_handler = exit_handler;
	sigaction(SIGTERM, &sa, NULL);

	sa.sa_handler = exit_handler;
	sigaction(SIGINT, &sa, NULL);

	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, NULL);

	nl = nl_handle_alloc();

	if (!nl) {
		log_msg_nl_perror("nl_handle_alloc");
		return 1;
	}

	log_msg_nl_perror("genl_connect");
	genl_connect(nl);
	log_msg_nl_perror("genl_connect");

	family = genl_ctrl_resolve(nl, IEEE80215_NL_NAME);
	log_msg_nl_perror("genl_ctrl_resolve");

	nl_socket_add_membership(nl, nl_get_multicast_id(nl, IEEE80215_NL_NAME, IEEE80215_MCAST_COORD_NAME));

	seq_expected = nl_socket_use_seq(nl) + 1;

	nl_socket_modify_cb(nl, NL_CB_VALID, NL_CB_CUSTOM, parse_cb, NULL);
	nl_socket_modify_cb(nl, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, seq_check, NULL);

#if 0
	if(debug == 0) {
		int f, c;
		close(0);
		close(1);
		close(2);
		c = 20;
		while(((f = fork()) == EAGAIN) && c) c--;
		if (f > 0) {
			return 0; /* Exiting parent */
		} else if (f < 0) {
			fprintf(stderr, "Out of memory\n");
			return -1; /* Error exit */
		}
	}
#endif
	uid = getuid();
	umask(022);

	if (uid == 0) {
		gid_t gid = getgid();

		/* If run by hand, ensure groups vector gets trashed */
		setgroups(1, &gid);
	}

	if (debug <= 0) {
		if (daemon(0, 0) < 0) {
			perror("daemon");
			return 2;
		}
	}
	pid_fd = open (PID_FILE, O_WRONLY | O_CREAT, 0640);
	if (pid_fd < 0) {
		perror ("open");
		return 1;
	} else {
		char buf[PID_BUF_LEN];
		int ret;
		snprintf (buf, sizeof(buf), "%d\n", getpid());
		ret = write (pid_fd, buf, strlen(buf));

		if (ret < 0) {
			perror ("write");
			return 1;
		}

		close (pid_fd);
	}
	mlme_start(short_addr, pan, 1, iface);

	while (!die_flag) {
		nl_recvmsgs_default(nl);
	}
	cleanup(0);

	return 0;
}

