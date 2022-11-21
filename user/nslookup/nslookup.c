//
// Created by Zhao Qi‘ao on 2022/11/15.
//
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>

int main(int argc, char *argv[]){
    /*
     * Usage:nslookup [-option] [name | -] [server]
     */
    if (argc < 2){
        perror("Usage Error\n");
        return -1;
    }
    struct sockaddr_in addr;
    struct hostent * hptr;
    if(inet_pton(AF_INET, argv[1], &addr.sin_addr) != 1){
        // Is Not IP Addr
        char *hostname = argv[1];
        if (!(hptr = gethostbyname(hostname))) {
            perror("Get HostName Error\n");
            return -1;
        }
        printf("NAME :    %s\n", hptr->h_name);
        char **p_alias = hptr->h_aliases;
        char **p_addr = hptr->h_addr_list;
        char str[INET_ADDRSTRLEN];
        while (*p_alias) {
            printf("ALIAS:    %s\n", *p_alias);
            p_alias++;
        }
        while (*p_addr) {
            if (hptr->h_addrtype == AF_INET) {
                printf("ADDR :    %s\n", inet_ntop(hptr->h_addrtype, *p_addr, str, sizeof(str)));
                p_addr++;
            } else {
                perror("Unknown Address Type\n");
            }
        }
    }
    else if (inet_pton(AF_INET, argv[1], &addr.sin_addr) == 1){
        if (!(hptr = gethostbyaddr((const char*)&addr.sin_addr, strlen(&addr.sin_addr), AF_INET))){
            perror("Get HostName Error\n");
            printf("Server cannot find %s",argv[1]);
            return -1;
        }
        printf("IP Address = %s          NAME = %s", argv[1], hptr->h_name);
    }
    return 0;
}