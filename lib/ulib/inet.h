//
// Created by bear on 11/17/2022.
//

#pragma once

#include <socket.h>
#include <user.h>

// additional functions
void hexdump(void *data, size_t size);
uint16_t hton16(uint16_t h);
uint16_t ntoh16(uint16_t n);
uint32_t hton32(uint32_t h);
uint32_t ntoh32(uint32_t n);

#define htons(x) hton16(x)
#define ntohs(x) ntoh16(x)
#define htonl(x) hton32(x)
#define ntohl(x) ntoh32(x)

int inet_aton(const char *cp, struct in_addr *ap);
char *inet_ntoa(struct in_addr in);
in_addr_t inet_addr(const char *cp);
