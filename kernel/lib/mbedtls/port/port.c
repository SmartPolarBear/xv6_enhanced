//
// Created by bear on 11/29/2022.
//
#include "types.h"
#include "defs.h"
#include "mbedtls/entropy.h"

void mbedtls_entropy_init(mbedtls_entropy_context *ctx)
{
	memset(ctx, 0, sizeof(mbedtls_entropy_context));

	for (int i = 0; i < MBEDTLS_ENTROPY_MAX_SOURCES; i++)
	{
		ctx->source[i].size = 0;
	}
}