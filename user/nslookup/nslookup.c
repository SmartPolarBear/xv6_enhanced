//
// Created by bear on 11/7/2022.
//

#include "types.h"
#include "netdb.h"
#include "user.h"

int main()
{
	struct hostent *host = (struct hostent *)gethostbyname("www.baidu.com");
	if (host == NULL)
	{
		printf(1, "gethostbyname failed");
	}

	printf(1, "h_name: %s", host->h_name);
	printf(1, "h_addr: %s", host->h_addr);
	return 0;
}