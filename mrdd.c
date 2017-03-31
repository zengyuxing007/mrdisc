/* mrdisc daemon
 *
 * Copyright (c) 2017  Joachim Nilsson <troglobit@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <err.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/igmp.h>
#include <sys/types.h>
#include <sys/socket.h>

#define IGMP_MRDISC_ANNOUNCE 0x30
#define IGMP_MRDISC_SOLICIT  0x31
#define IGMP_MRDISC_TERM     0x32
#define MC_ALL_SNOOPERS      "224.0.0.106"

static void compose_addr(struct sockaddr_in *sin, char *group)
{
	memset(sin, 0, sizeof(*sin));
	sin->sin_family      = AF_INET;
	sin->sin_addr.s_addr = inet_addr(group);
}

static unsigned short in_cksum (unsigned short *addr, int len)
{
   register int sum = 0;
   u_short answer = 0;
   register u_short *w = addr;
   register int nleft = len;

   /*
    * Our algorithm is simple, using a 32 bit accumulator (sum), we add
    * sequential 16 bit words to it, and at the end, fold back all the
    * carry bits from the top 16 bits into the lower 16 bits.
    */
   while (nleft > 1)
   {
      sum += *w++;
      nleft -= 2;
   }

   /* mop up an odd byte, if necessary */
   if (nleft == 1)
   {
      *(u_char *) (&answer) = *(u_char *) w;
      sum += answer;
   }

   /* add back carry outs from top 16 bits to low 16 bits */
   sum = (sum >> 16) + (sum & 0xffff);  /* add hi 16 to low 16 */
   sum += (sum >> 16);           /* add carry */
   answer = ~sum;                /* truncate to 16 bits */

   return answer;
}

int main(void)
{
	char loop;
	int sd, val, rc;
	size_t num;
	struct igmp igmp;
	struct sockaddr dest;
	unsigned char ra[4] = { IPOPT_RA, 0x04, 0x00, 0x00 };

	sd = socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);
	if (sd < 0)
		err(1, "Can't open socket");

	val = 1;
	rc = setsockopt(sd, IPPROTO_IP, IP_MULTICAST_TTL, &val, sizeof(val));
	if (rc < 0)
		err(1, "Can't set TTL");

	loop = 0;
	rc = setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
	if (rc < 0)
		err(1, "Can't disable MC loop");

	rc = setsockopt(sd, IPPROTO_IP, IP_OPTIONS, &ra, sizeof(ra));
	if (rc < 0)
		err(1, "Can't set IP OPTIONS");

	compose_addr((struct sockaddr_in *)&dest, MC_ALL_SNOOPERS);

	memset(&igmp, 0, sizeof(igmp));
	igmp.igmp_type = IGMP_MRDISC_ANNOUNCE;
	igmp.igmp_code = 20;	/* 4-180 sec, default 20 sec */
	igmp.igmp_cksum = in_cksum((unsigned short *)&igmp, sizeof(igmp));

	while (1) {
		num = sendto(sd, &igmp, sizeof(igmp), 0, &dest, sizeof(dest));
		if (num < 0)
			err(1, "Can't send Membership report message.");
		sleep(20);
	}

	return close(sd);
}

/**
 * Local Variables:
 *  compile-command: "make mrdd"
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */