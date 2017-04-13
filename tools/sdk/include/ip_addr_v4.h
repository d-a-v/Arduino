
#ifndef __IPV4_ADDR_H
#define __IPV4_ADDR_H

#include "lwip/init.h" // LWIP_VERSION_
#if LWIP_VERSION_MAJOR == 1
#define ip_addr_v4	ip_addr
#define ip_addr_v4_t	ip_addr_t
#include "lwip/ip_addr.h"
#else
// lwip1 ip_addr definition renamed ip_addr_v4
typedef struct ip_addr_v4
{
	uint32_t addr;
} ip_addr_v4_t;
// struct ip_info is not lwip, was introduced by esp in lwip1/ip_addr.h
struct ip_info {
    struct ip_addr_v4 ip;
    struct ip_addr_v4 netmask;
    struct ip_addr_v4 gw;
};
#endif

#endif // __IPV4_ADDR_H
