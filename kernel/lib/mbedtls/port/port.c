//
// Created by bear on 11/29/2022.
//
#include "types.h"
#include "defs.h"
#include "date.h"
#include "memory.h"

#include "mbedtls/entropy.h"
#include "mbedtls/platform.h"

void mbedtls_entropy_init(mbedtls_entropy_context *ctx)
{
	memset(ctx, 0, sizeof(mbedtls_entropy_context));

	for (int i = 0; i < MBEDTLS_ENTROPY_MAX_SOURCES; i++)
	{
		ctx->source[i].size = 0;
	}
}

mbedtls_time_t mbedtls_time_impl(mbedtls_time_t *timer)
{
	struct rtcdate r;
	cmostime(&r);
	*timer = unixime_in_seconds(&r);
	return *timer;
}

void *mbedtls_calloc_impl(size_t nmemb, size_t size)
{
	char *ret = kmalloc(nmemb * size, 0);
	memset(ret, 0, nmemb * size);
	return ret;
}

void mbedtls_free_impl(void *mem)
{
	kfree(mem);
}