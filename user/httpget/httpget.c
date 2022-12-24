#include <user.h>
#include <netdb.h>
#include <socket.h>
#include <inet.h>

// httpget mba.xmu.edu.cn/content/page/84.html
char host[64], buf[512];

int main(int argc, char **argv)
{
	const char *url, *path;
	ssize_t i, n;
	struct hostent *hp;
	int sockfd, r;
	struct sockaddr_in addr = {
		.sin_family = PF_INET, .sin_port = htons(80),
	};

	if (argc < 2)
	{
		printf(2, "usage: httpget url\n");
		return 0;
	}
	url = argv[1];

	path = strchr(url, '/');
	if (path)
	{
		memmove(host, url, path - url);
		host[path - url] = 0;
	}
	else
	{
		strcpy(host, url);
		path = "/";
	}

	printf(1, "host: %s, path: %s\n", host, path);
	hp = (struct hostent *)gethostbyname(host);
	//assert(hp, "gethostbyname");
	addr.sin_addr.s_addr = *(uint32_t *)hp->h_addr_list[0];
	printf(1, "* Trying %s (%s)\n", hp->h_name, inet_ntoa(*(struct in_addr *)&addr.sin_addr));

	sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	//assert(sockfd >= 0, "socket");

	r = connect(sockfd, (const struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	//assert(r == 0, "connect");

	/* send http request */
	snprintf(buf, sizeof(buf), "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: curl\r\n\r\n", path, host);
	for (i = 0, n = strlen(buf); i < n;)
	{
		r = send(sockfd, buf + i, n - i);
		//assert(r > 0, "send");
		i += r;
	}
	write(1, buf, n);

	/* print http response */
	while (1)
	{
		n = recv(sockfd, buf, sizeof(buf));
		if (n <= 0)
		{
			break;
		}
		write(1, buf, n);
	}

	close(sockfd);
	return 0;
}
