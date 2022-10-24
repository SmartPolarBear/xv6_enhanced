//
// Created by bear on 10/24/2022.
//
#pragma once

#include "types.h"

#define NNETCARDNAME 16

typedef struct netcard_opts
{
	int (*init)(void *, void *);
	int (*send)(void *, const void *, int);
	int (*receive)(void *, void *, int);
} netcard_opts_t;

typedef struct netcard
{
	char name[NNETCARDNAME];
	list_head_t link;

	uint32 id;
	struct pci_func *func;
	struct netcard_opts *opts;

	void *prvt;
} netcard_t;