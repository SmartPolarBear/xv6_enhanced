//
// Created by bear on 10/29/2022.
//
#include "types.h"
#include "defs.h"

#include "socket.h"

void socketinit(void)
{
	
}

struct file *socketalloc(int domain, int type, int protocol)
{}

void socketclose(struct socket *)
{}

int socketconnect(struct socket *, struct sockaddr *, int)
{}

int socketbind(struct socket *, struct sockaddr *, int)
{}

int socketlisten(struct socket *, int)
{}

struct file *socketaccept(struct socket *, struct sockaddr *, int *)
{}

int socketread(struct socket *, char *, int)
{}

int socketwrite(struct socket *, char *, int)
{}

int socketrecvfrom(struct socket *, char *, int, struct sockaddr *, int *)
{}

int socketsendto(struct socket *, char *, int, struct sockaddr *, int)
{}

int socketioctl(struct socket *, int, void *)
{}