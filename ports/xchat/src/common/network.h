typedef struct netstore_
{
#ifdef NETWORK_PRIVATE
#ifdef USE_IPV6
	struct addrinfo *ip6_hostent;
#else
	struct hostent *ip4_hostent;
	struct sockaddr_in addr;
#endif
#endif
} netstore;

#define MAX_HOSTNAME 256

netstore *net_store_new (void);
void net_store_destroy (netstore *ns);
int net_connect (netstore *ns, int sok4, int sok6, int *sok_return);
char *net_resolve (netstore *ns, char *hostname, int port, char **real_host);
void net_bind (netstore *tobindto, int sok4, int sok6);
char *net_ip (unsigned long addr);
void net_sockets (int *sok4, int *sok6);
