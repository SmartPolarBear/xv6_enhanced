//
// Created by bear on 11/1/2022.
//

#include "socket.h"
#include "user.h"


/* RFC 867: Daytime Protocol */
#define SERVER_HOST "utcnist.colorado.edu"
#define SERVER_PORT 13

int main(int argc, char **argv)
{
	struct hostent *hp;
	int sockfd, r;
	struct sockaddr_in addr = {
		.sin_family = PF_INET, .sin_port = hton16(SERVER_PORT),
	};

	addr.sin_addr.addr = 0x2c8c8a80;

	sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	printf(1, "socket\n");

	printf(1, "connecting\n");
	r = connect(sockfd, (const struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	if (r < 0)
	{
		printf(1, "daytime: connect failed: %d\n", r);
		exit(-1);
	}
	printf(1, "connected\n");

	while (1)
	{
		char buf[512];
		ssize_t n;

		n = recv(sockfd, buf, sizeof(buf));
		printf(1, "received");

		if (n <= 0)
		{
			break;
		}
		write(1, buf, n);
	}

	close(sockfd);
	printf(1, "closed");

	return 0;
}