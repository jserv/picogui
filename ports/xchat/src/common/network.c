/* X-Chat
 * Copyright (C) 2001 Peter Zelezny.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

/* ipv4 and ipv6 networking functions with a common interface */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define WANTSOCKET
#define WANTARPA
#define WANTDNS
#include "inet.h"

#include "../../config.h"				  /* grab USE_IPV6 define */

#define NETWORK_PRIVATE
#include "network.h"


/* ================== COMMON ================= */

static void
net_set_socket_options (int sok)
{
	int sw;

	sw = 1;
	setsockopt (sok, SOL_SOCKET, SO_REUSEADDR, (char *) &sw, sizeof (sw));
	sw = 1;
	setsockopt (sok, SOL_SOCKET, SO_KEEPALIVE, (char *) &sw, sizeof (sw));
}

char *
net_ip (unsigned long addr)
{
	struct in_addr ia;

	ia.s_addr = htonl (addr);
	return inet_ntoa (ia);
}

void
net_store_destroy (netstore * ns)
{
#ifdef USE_IPV6
	if (ns->ip6_hostent)
		freeaddrinfo (ns->ip6_hostent);
#endif
	free (ns);
}

netstore *
net_store_new (void)
{
	netstore *ns;

	ns = malloc (sizeof (netstore));
	memset (ns, 0, sizeof (netstore));

	return ns;
}

#ifndef USE_IPV6

/* =================== IPV4 ================== */

char *
net_resolve (netstore * ns, char *hostname, int port, char **real_host)
{
	ns->ip4_hostent = gethostbyname (hostname);
	if (!ns->ip4_hostent)
		return NULL;

	memset (&ns->addr, 0, sizeof (ns->addr));
	memcpy (&ns->addr.sin_addr, ns->ip4_hostent->h_addr,
			  ns->ip4_hostent->h_length);
	ns->addr.sin_port = htons (port);
	ns->addr.sin_family = AF_INET;

	*real_host = strdup (ns->ip4_hostent->h_name);
	return strdup (inet_ntoa (ns->addr.sin_addr));
}

int
net_connect (netstore * ns, int sok4, int sok6, int *sok_return)
{
	*sok_return = sok4;
	return connect (sok4, (struct sockaddr *) &ns->addr, sizeof (ns->addr));
}

void
net_bind (netstore * tobindto, int sok4, int sok6)
{
	bind (sok4, (struct sockaddr *) &tobindto->addr, sizeof (tobindto->addr));
}

void
net_sockets (int *sok4, int *sok6)
{
	*sok4 = socket (AF_INET, SOCK_STREAM, 0);
	*sok6 = -1;
	net_set_socket_options (*sok4);
}

#else

/* =================== IPV6 ================== */

char *
net_resolve (netstore * ns, char *hostname, int port, char **real_host)
{
	struct addrinfo hints;
	char ipstring[MAX_HOSTNAME];
	char portstring[MAX_HOSTNAME];
	int ret;

/*	if (ns->ip6_hostent)
		freeaddrinfo (ns->ip6_hostent);*/

	sprintf (portstring, "%d", port);

	memset (&hints, 0, sizeof (struct addrinfo));
	hints.ai_family = PF_UNSPEC; /* support ipv6 and ipv4 */
	hints.ai_flags = AI_CANONNAME;
	hints.ai_socktype = SOCK_STREAM;

	if (port == 0)
		ret = getaddrinfo (hostname, NULL, &hints, &ns->ip6_hostent);
	else
		ret = getaddrinfo (hostname, portstring, &hints, &ns->ip6_hostent);
	if (ret != 0)
		return NULL;

	/* find the numeric IP number */
	ipstring[0] = 0;
	getnameinfo (ns->ip6_hostent->ai_addr, ns->ip6_hostent->ai_addrlen,
					 ipstring, sizeof (ipstring), NULL, 0, NI_NUMERICHOST);

	if (ns->ip6_hostent->ai_canonname)
		*real_host = strdup (ns->ip6_hostent->ai_canonname);
	else
		*real_host = strdup (hostname);

	return strdup (ipstring);
}

/* the only thing making this interface unclean, this shitty sok4, sok6 business */

int
net_connect (netstore * ns, int sok4, int sok6, int *sok_return)
{
	struct addrinfo *res, *res0;
	int error = -1;

	res0 = ns->ip6_hostent;

	for (res = res0; res; res = res->ai_next)
	{
/*		sok = socket (res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sok < 0)
			continue;*/
		switch (res->ai_family)
		{
		case AF_INET:
			error = connect (sok4, res->ai_addr, res->ai_addrlen);
			*sok_return = sok4;
			break;
		case AF_INET6:
			error = connect (sok6, res->ai_addr, res->ai_addrlen);
			*sok_return = sok6;
			break;
		default:
			error = 1;
		}

		if (error == 0)
			break;
	}

	return error;
}

void
net_bind (netstore * tobindto, int sok4, int sok6)
{
	bind (sok4, tobindto->ip6_hostent->ai_addr,
			tobindto->ip6_hostent->ai_addrlen);
	bind (sok6, tobindto->ip6_hostent->ai_addr,
			tobindto->ip6_hostent->ai_addrlen);
}

void
net_sockets (int *sok4, int *sok6)
{
	*sok4 = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
	*sok6 = socket (AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	net_set_socket_options (*sok4);
	net_set_socket_options (*sok6);
}

#endif
