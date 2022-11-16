//
// Created by bear on 11/7/2022.
//

#include "types.h"
#include "netdb.h"
#include "user.h"

int main()
{
	struct hostent *host = (struct hostent *)gethostbyname("cn.bing.com");
	if (host == NULL)
	{
		printf(1, "gethostbyname failed");
	}

	printf(1, "h_name: %s\n", host->h_name);
	printf(1, "h_addr: 0x%x\n", *((uint32_t *)(host->h_addr_list[0])));
	return 0;
}