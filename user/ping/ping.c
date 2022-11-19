#include <user.h>
#include <netdb.h>
#include <socket.h>
#include <inet.h>

typedef int SOCKET;
typedef struct ICMP_HDR
{
	unsigned char icmp_type; // 消息类型
	unsigned char icmp_code; // 代码
	unsigned short icmp_checksum; // 校验和
	// 下面是回显头
	unsigned short icmp_id; // 用来惟一标识此请求的ID号
	unsigned short icmp_sequence; // 序列号
	unsigned long icmp_timestamp; // 时间戳
} ICMP_HDR;
typedef struct IPHeader
{// 20字节的IP头
	uint8_t iphVerLen; // 版本号和头长度（各占4位）
	uint8_t ipTOS; // 服务类型
	uint16_t ipLength; // 封包总长度，即整个IP报的长度
	uint16_t ipID;  // 封包标识，惟一标识发送的每一个数据报
	uint16_t ipFlags; // 标志
	uint8_t ipTTL; // 生存时间，就是TTL
	uint8_t ipProtocol; // 协议，可能是TCP、UDP、ICMP等
	uint16_t ipChecksum; // 校验和
	uint32_t ipSource; // 源IP地址
	uint32_t ipDestination; // 目标IP地址
} IPHeader;
uint32_t GetIpByName(const char *name)
{
	struct hostent *pHostent = (struct hostent *)gethostbyname(name);
	if (pHostent == NULL)
	{
		return INADDR_NONE;
	}
	uint32_t host = *(uint32_t *)pHostent->h_addr_list[0];
	return host;
}

int SetTimeout(SOCKET s, int nTime, int bRecv)
{
	int ret = setsockopt(s, SOL_SOCKET, bRecv ? SO_RCVTIMEO : SO_SNDTIMEO, (char *)&nTime, sizeof(nTime));
	return ret != -1;
}

uint16_t checksum(uint16_t *buff, int size)
{
	unsigned long cksum = 0;
	while (size > 1)
	{
		cksum += *buff++;
		size -= sizeof(uint16_t);
	}
	// 是奇数
	if (size)
	{
		cksum += *(uint8_t *)buff;
	}
	// 将32位的chsum高16位和低16位相加，然后取反
	while (cksum >> 16)
		cksum = (cksum >> 16) + (cksum & 0xffff);
	return (uint16_t)(~cksum);
}

int ping(in_addr_t ip, uint16_t nSeq)
{

	uint16_t pID = (uint16_t)time(0);
	// 创建原始套节字
	SOCKET sRaw = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sRaw == -1)
	{
		int eno = errno;
		if (eno == 1)
		{
			// 必须使用sudo才能建立Raw socket
			printf(1, "Operation not permitted!\n");
		}
		else
		{
			printf(1, "Cannot create socket! Error %d", eno);
		}
		return -1;
	}
	// 设置接收超时
	if (SetTimeout(sRaw, 1000, true))
	{
		printf(1, "Cannot set timeout!\n");
	}
	// 设置目的地址
	struct sockaddr_in dest;
	dest.sin_family = AF_INET;
	dest.sin_port = htons(0);
	dest.sin_addr.s_addr = ip;
	// 创建ICMP封包
	char buff[sizeof(ICMP_HDR) + 32];
	ICMP_HDR *pIcmp = (ICMP_HDR *)buff;
	// 填写ICMP封包数据
	pIcmp->icmp_type = 8; // 请求一个ICMP回显
	pIcmp->icmp_code = 0;
	pIcmp->icmp_id = (uint16_t)pID;
	pIcmp->icmp_checksum = 0;
	pIcmp->icmp_sequence = 0;
	// 填充数据部分，可以为任意
	memset(&buff[sizeof(ICMP_HDR)], 'E', 32);
	// 开始发送和接收ICMP封包

	char recvBuf[1024];
	struct sockaddr_in from;
	socklen_t nLen = sizeof(from);

	static int nCount = 0;
	long nRet;

	pIcmp->icmp_checksum = 0;
	pIcmp->icmp_timestamp = clock();
	pIcmp->icmp_sequence = nSeq++;
	pIcmp->icmp_checksum = checksum((uint16_t *)buff, sizeof(ICMP_HDR) + 32);
	nRet = (long)sendto(sRaw, buff, sizeof(ICMP_HDR) + 32,
						0, (struct sockaddr *)&dest, sizeof(dest));
	if (nRet == -1)
	{
		printf(1, " sendto() failed: %d \n", errno);
		return -1;
	}
	nRet = (long)recvfrom(sRaw, recvBuf, 1024, 0, (struct sockaddr *)&from, &nLen);
	if (nRet == -1)
	{
		printf(1, " recvfrom() failed: %d\n", errno);
		return -1;
	}
	// 下面开始解析接收到的ICMP封包
	clock_t nTick = clock();
	if (nRet < sizeof(IPHeader) + sizeof(ICMP_HDR))
	{
		printf(1, " Too few bytes from %s \n", inet_ntoa(from.sin_addr));
	}
	// 接收到的数据中包含IP头，IP头大小为20个字节，所以加20得到ICMP头
	ICMP_HDR *pRecvIcmp = (ICMP_HDR *)(recvBuf + 20); // (ICMP_HDR*)(recvBuf + sizeof(IPHeader));
	IPHeader *ipHeader = (IPHeader *)recvBuf;
	if (pRecvIcmp->icmp_type != 0)// 回显
	{
		printf(1, " nonecho type %d recvd \n", pRecvIcmp->icmp_type);
		return -1;
	}
	if (pRecvIcmp->icmp_id != pID)
	{
		printf(1, " someone else's packet! \n");
		return -1;
	}
	printf(1, " %d bytes from %s:", (int)nRet, inet_ntoa(from.sin_addr));
	printf(1, " icmp_seq = %d. ", pRecvIcmp->icmp_sequence);
	printf(1, " ttl = %d. ", ipHeader->ipTTL);
	printf(1, " time: %d ms", (int)nTick - (int)pRecvIcmp->icmp_timestamp);
	printf(1, " \n");

	return 0;
}

int main(int args, const char *argv[])
{
	// 目的IP地址，即要Ping的IP地址
	const char *szDestIp = (args > 1) ? argv[1] : "127.0.0.1";
	in_addr_t ip = inet_addr(szDestIp);
	if ((int)ip == -1)
	{
		ip = GetIpByName(szDestIp);
	} // nds
	if ((int)ip == -1)
	{
		printf(1, "invalid ip address:%s\n", szDestIp);
		return -1;
	}
	printf(1, "ping %s\n", szDestIp);
	for (int i = 0; i < 4; ++i)
	{
		if (ping(ip, i))
		{
			return -1;
		}
	}
	return 0;
}
