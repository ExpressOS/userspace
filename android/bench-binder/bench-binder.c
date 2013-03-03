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

#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>

#include "binder.h"

static const size_t BINDER_VM_SIZE   = 4096 * 1024;
static const int SRV_PINGPONG        = 1;
static const int TEST_TIMES          = 10000;
static const size_t VM_SIZE          = 128 * 1024;
static const size_t MAX_PAYLOAD_SIZE = 16384;

static struct binder_state *bs;
static void *ping_thr(void *);

static int pong_handler(struct binder_state *bs,
                        struct binder_txn *txn,
                        struct binder_io *msg,
                        struct binder_io *reply);

static unsigned pong_token;
static int svcmgr_publish(struct binder_state *bs, void *target, const char *name, void *ptr);
static void *svcmgr_lookup(struct binder_state *bs, void *target, const char *name);

static const char *service_name = "me.haohui.expressos.binderbench";
static void start_ping(int buffer_size);

int main()
{
        bs = binder_open(VM_SIZE);

        int ret = svcmgr_publish(bs, BINDER_SERVICE_MANAGER, service_name, &pong_token);
        if (ret < 0) {
                printf("Failed to register service\n");
                return 1;
        }

        if (fork() != 0) {
                // parent
                binder_loop(bs, pong_handler);                
        } else {
                binder_close(bs);
                bs = binder_open(VM_SIZE);
                start_ping(4);
                start_ping(1024);
                start_ping(2048);
                start_ping(4096);
                start_ping(8192);
                start_ping(16384);
        }
        

        return 0;
}

static int pong_handler(struct binder_state *bs,
                        struct binder_txn *txn,
                        struct binder_io *msg,
                        struct binder_io *reply)
{
        uint16_t *p = bio_get_string16(msg, NULL);
	bio_put_string16(reply, p);
        return 0;
}

static void start_ping(int size)
{
	uint16_t mock_payload[MAX_PAYLOAD_SIZE / sizeof(uint16_t)];
	
        void *pong_srv = svcmgr_lookup(bs, BINDER_SERVICE_MANAGER, service_name);
        int i = 0;
        unsigned status;
        unsigned iodata[MAX_PAYLOAD_SIZE + 512/4];
        struct binder_io msg, reply;

        struct timeval tv_start, tv_end;
	memset(mock_payload, 1, sizeof(mock_payload));
	mock_payload[size / sizeof(uint16_t)] = 0;

        gettimeofday(&tv_start, NULL);
        for (i = 0; i < TEST_TIMES; ++i) {
                bio_init(&msg, iodata, sizeof(iodata), 4);
                bio_put_string16(&msg, mock_payload);
                if (binder_call(bs, &msg, &reply, pong_srv, SRV_PINGPONG)) {
                        printf("start_ping: binder_call failed, i=%d\n", i);
                        return;
                }
//                status = bio_get_uint32(&reply);
                binder_done(bs, &msg, &reply);
        }
        gettimeofday(&tv_end, NULL);
        printf("pingpong time (%d times, size=%d) : %ld ms\n", TEST_TIMES, size,
               (tv_end.tv_sec - tv_start.tv_sec) * 1000 +
               (tv_end.tv_usec - tv_start.tv_usec) / 1000);
}

static int svcmgr_publish(struct binder_state *bs, void *target, const char *name, void *ptr)
{
    unsigned status;
    unsigned iodata[512/4];
    struct binder_io msg, reply;

    bio_init(&msg, iodata, sizeof(iodata), 4);
    bio_put_uint32(&msg, 0);  // strict mode header
    bio_put_string16_x(&msg, SVC_MGR_NAME);
    bio_put_string16_x(&msg, name);
    bio_put_obj(&msg, ptr);

    if (binder_call(bs, &msg, &reply, target, SVC_MGR_ADD_SERVICE))
        return -1;

    status = bio_get_uint32(&reply);

    binder_done(bs, &msg, &reply);

    return status;
}

static void *svcmgr_lookup(struct binder_state *bs, void *target, const char *name)
{
    void *ptr;
    unsigned iodata[512/4];
    struct binder_io msg, reply;

    bio_init(&msg, iodata, sizeof(iodata), 4);
    bio_put_uint32(&msg, 0);  // strict mode header
    bio_put_string16_x(&msg, SVC_MGR_NAME);
    bio_put_string16_x(&msg, name);

    if (binder_call(bs, &msg, &reply, target, SVC_MGR_GET_SERVICE))
        return 0;

    ptr = bio_get_ref(&reply);

    if (ptr)
        binder_acquire(bs, ptr);

    binder_done(bs, &msg, &reply);

    return ptr;
}
