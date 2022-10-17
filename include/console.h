//
// Created by bear on 10/17/2022.
//

#pragma once

typedef struct consolectl
{
	struct spinlock lock;
	int locking;
} consolectl_t;