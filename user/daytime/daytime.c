//
// Created by bear on 11/1/2022.
//

#include "socket.h"
#include "user.h"


/* RFC 867: Daytime Protocol */
#define SERVER_HOST "utcnist.colorado.edu"
#define SERVER_PORT 13
#define SERVER_HOSTIP 0x2c8c8a80
char buf[512];

int main(int argc, char **argv)
{
	struct hostent *hp;
	int sockfd, r;
	struct sockaddr_in addr = {
		.sin_family = PF_INET, .sin_port =  hton16(SERVER_PORT),
	};

	addr.sin_addr.s_addr = hton32(SERVER_HOSTIP);

	sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	r = connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	if (r < 0)
	{
		printf(1, "daytime: connect failed: %d\n", r);
		exit(-1);
	}

	while (1)
	{
		ssize_t n;

		n = recv(sockfd, buf, sizeof(buf));
		if (n <= 0)
		{
			break;
		}

		write(1, buf, n); // stdout
	}

	close(sockfd);

	return 0;
}