//
// Created by Zhao Qi‘ao on 2022/11/15.
//
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <math.h>
#include <netdb.h>
#define MAX_BUF_SIZE 1024
#define TYPE_A 1
#define DNS_SERVER_PORT 53
#define DNS_SERVER_IP "114.114.114.114"
#define CNAME 0x05

int main(int argc, char *argv[]){
    /*
     * Usage:nslookup [-option] [name | -] [server]
     */
    if (argc < 2){
        perror("Usage Error\n");
        return -1;
    }
    char* hostname = argv[1];

    struct hostent * hptr;
    if(!(hptr = gethostbyname(hostname))){
        perror("Get HostName Error\n");
        return -1;
    }
    printf("HOST NAME:    %s\n",hptr->h_name);
    char** p_alias = hptr->h_aliases;
    char** p_addr = hptr->h_addr_list;
    char str[INET_ADDRSTRLEN];
    while(*p_alias){
        printf("ALIAS:    %s\n",*p_alias);
        p_alias ++;
    }
    while(*p_addr){
        if (hptr->h_addrtype == AF_INET){
            printf("ADDR :    %s\n", inet_ntop(hptr->h_addrtype, *p_addr, str, sizeof(str)));
            p_addr ++;
        }
        else{
            perror("Unknown Address Type\n");
        }
    }
    return 0;
}