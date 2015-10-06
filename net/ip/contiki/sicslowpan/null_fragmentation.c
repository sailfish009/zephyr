/* compression.h - Header Compression */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <net/net_buf.h>
#include <net/net_core.h>

#include <net/sicslowpan/null_fragmentation.h>
#include <net/netstack.h>

#include "contiki/ipv6/uip-ds6-nbr.h"

#define DEBUG 0
#include "net/ip/uip-debug.h"

#if UIP_LOGGING
#include <stdio.h>
void uip_log(char *msg);
#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif

static void
packet_sent(struct net_mbuf *buf, void *ptr, int status, int transmissions)
{
	const linkaddr_t *dest = packetbuf_addr(buf, PACKETBUF_ADDR_RECEIVER);
	uip_ds6_link_neighbor_callback(dest, status, transmissions);
	uip_last_tx_status(buf) = status;
}

static int fragment(struct net_buf *buf, void *ptr)
{
	struct net_mbuf *mbuf;
	int ret;

	mbuf = net_mbuf_get_reserve(0);
	if (!mbuf) {
		return 0;
	}

	NET_BUF_CHECK_IF_NOT_IN_USE(buf);

	ret = packetbuf_copyfrom(mbuf, &uip_buf(buf)[UIP_LLH_LEN],
				 uip_len(buf));
	PRINTF("%s: buffer len %d copied %d\n", __FUNCTION__, uip_len(buf), ret);
	packetbuf_set_addr(mbuf, PACKETBUF_ADDR_RECEIVER, &buf->dest);
	net_buf_put(buf);

	return NETSTACK_LLSEC.send(mbuf, &packet_sent, true, ptr);
}

static int send_upstream(struct net_buf *buf)
{
	if (!NETSTACK_COMPRESS.uncompress(buf)) {
		UIP_LOG("Uncompression failed.");
		return -EINVAL;
	}

	if (net_recv(buf) < 0) {
		UIP_LOG("Input to IP stack failed.");
		return -EINVAL;
	}

	return 0;
}

static int reassemble(struct net_mbuf *mbuf)
{
        struct net_buf *buf;

	buf = net_buf_get_reserve_rx(0);
	if (!buf) {
		return 0;
	}

	if(packetbuf_datalen(mbuf) > 0 &&
		packetbuf_datalen(mbuf) <= UIP_BUFSIZE - UIP_LLH_LEN) {
		memcpy(uip_buf(buf), packetbuf_dataptr(mbuf),
						packetbuf_datalen(mbuf));
		uip_len(buf) = packetbuf_datalen(mbuf);

		if (send_upstream(buf) < 0) {
			net_buf_put(buf);
		} else {
			net_mbuf_put(mbuf);
		}

		return 1;
        } else {
                PRINTF("packet discarded, datalen %d MAX %d\n",
				packetbuf_datalen(mbuf), UIP_BUFSIZE - UIP_LLH_LEN);
		net_buf_put(buf);
                return 0;
        }
}

const struct fragmentation null_fragmentation = {
	fragment,
	reassemble
};
