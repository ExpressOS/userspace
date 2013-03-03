/*
 * Copyright (c) 2012-2013 University of Illinois at
 * Urbana-Champaign. All rights reserved.
 *
 * Developed by:
 *
 *     Haohui Mai
 *     University of Illinois at Urbana-Champaign
 *     http://haohui.me
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal with the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimers.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimers in the documentation and/or other materials
 *      provided with the distribution.
 *
 *    * Neither the names of University of Illinois at
 *      Urbana-Champaign, nor the names of its contributors may be
 *      used to endorse or promote products derived from this Software
 *      without specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS WITH THE SOFTWARE.
 */

/*
 * A library that intercepts all Binder IPC calls.
 */
#include <dlfcn.h>
#include <stdint.h>
#include <sys/types.h>
#include <linux/binder.h>
#include <stdio.h>
#include <pthread.h>

static void dump_memory(const void * loc, size_t size);
static void dump_transaction(unsigned long buffer, unsigned long offset, unsigned long write_size);
static int initialized = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int (*old_ioctl)(int, unsigned long, void*);

static void init()
{
	if (__sync_bool_compare_and_swap(&initialized, 0, 1)) {
		old_ioctl = dlsym(RTLD_NEXT, "ioctl");
	}
}
 
int ioctl(int fd, unsigned long request, void * arg)
{
	init();
	while (!initialized);
	int ret = 0;

        struct binder_write_read * p_bwr = (struct binder_write_read *)arg;

        if (request != BINDER_WRITE_READ)
                goto out;

        
	pthread_mutex_lock(&mutex);
        printf("=== tid=%d bwr.write_size = %ld, bwr.write_consumed = %ld ===\n",
               gettid(), p_bwr->write_size, p_bwr->write_consumed);

        if (p_bwr->write_size)
             dump_memory((const unsigned char *)p_bwr->write_buffer, p_bwr->write_size);
        printf ("====\n");
	dump_transaction(p_bwr->write_buffer, p_bwr->write_consumed, p_bwr->write_size);
	pthread_mutex_unlock(&mutex);


out:
        ret = old_ioctl(fd, request, arg);

	pthread_mutex_lock(&mutex);

	if (request == BINDER_WRITE_READ) {
           printf ("tid=%d === bwr.read_consumed=%lx === \n", gettid(), p_bwr->read_consumed);
           dump_memory((const unsigned char *)p_bwr->read_buffer, p_bwr->read_consumed);
	   dump_transaction(p_bwr->read_buffer, 0, p_bwr->read_consumed);
           printf ("====\n");
        }
	pthread_mutex_unlock(&mutex);
	return ret;
	
}

static void dump_memory(const void * loc, size_t size)
{
        size_t i;
        for (i = 1; i <= (size + 3) / 4; ++i) {
                printf("%08x ", ((const unsigned int*)loc)[i - 1]);
                if (i % 4 == 0)
                        printf("\n");
        }
	printf("\n");
}

static void dump_transaction(unsigned long buffer, unsigned long offset, unsigned long size) 
{
        unsigned int * p_cmd;
        for (p_cmd = (unsigned int *)(buffer + offset); p_cmd < (unsigned int *)(buffer + size); ++p_cmd) {
                if (*p_cmd != BC_TRANSACTION && *p_cmd != BR_REPLY)
                        continue;
                
                struct binder_transaction_data * tr = (struct binder_transaction_data*)(p_cmd + 1);

                printf("== tr->data->buffer, size = %d == \n", tr->data_size);
                dump_memory(tr->data.ptr.buffer, tr->data_size);
                printf("== tr->data->offset, size = %d == \n", tr->offsets_size);
                dump_memory(tr->data.ptr.offsets, tr->offsets_size);
        }
}

