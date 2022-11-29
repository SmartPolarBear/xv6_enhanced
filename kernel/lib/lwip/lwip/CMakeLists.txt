cmake_minimum_required(VERSION 3.22)

target_sources(kernel
        PRIVATE core/init.c
        PRIVATE core/def.c
        PRIVATE core/dns.c
        PRIVATE core/inet_chksum.c
        PRIVATE core/ip.c
        PRIVATE core/mem.c
        PRIVATE core/memp.c
        PRIVATE core/netif.c
        PRIVATE core/pbuf.c
        PRIVATE core/raw.c
        PRIVATE core/stats.c
        PRIVATE core/sys.c
        PRIVATE core/tcp.c
        PRIVATE core/tcp_in.c
        PRIVATE core/tcp_out.c
        PRIVATE core/timeouts.c
        PRIVATE core/udp.c
        PRIVATE core/ipv4/autoip.c
        PRIVATE core/ipv4/dhcp.c
        PRIVATE core/ipv4/etharp.c
        PRIVATE core/ipv4/icmp.c
        PRIVATE core/ipv4/igmp.c
        PRIVATE core/ipv4/ip4_frag.c
        PRIVATE core/ipv4/ip4.c
        PRIVATE core/ipv4/ip4_addr.c
        PRIVATE api/err.c
        PRIVATE netif/ethernet.c)

target_sources(kernel
        PRIVATE core/altcp.c
        PRIVATE core/altcp_tcp.c
        PRIVATE core/altcp_alloc.c
        PRIVATE apps/altcp_tls/altcp_tls_mbedtls.c
        PRIVATE apps/altcp_tls/altcp_tls_mbedtls_mem.c)

target_include_directories(kernel
        PRIVATE include)