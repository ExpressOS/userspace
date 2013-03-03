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

#include "vbinder.h"

#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>

#define LOG_TAG "bench-vbinder"

#include <cutils/log.h>

static void *pong_thread(void *);
static void *send_thread(void *);
static int pong_thr_tid;
static int ping_thr_tid;

static const int CHANNEL_LABEL = 1;
static const int TEST_TIMES = 10000;
static const size_t MAX_PAYLOAD_SIZE = 16384;

static pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

static void start_ping(int cap, int size);

int main(int argc, char *argv[])
{
        pthread_t pong_thr;

        int ping_cap = vbinder_create_channel(CHANNEL_LABEL, 1);
        ping_thr_tid = gettid();

        pthread_create(&pong_thr, NULL, pong_thread, NULL);
        sleep(3);

        int cap = vbinder_acquire_channel(pong_thr_tid, CHANNEL_LABEL);
        printf("vbinder_acquire_channel: pong_thr_tid=%x, cap=%d\n", pong_thr_tid, cap);
        fflush(stdout);

        start_ping(cap, 4);
        start_ping(cap, 1024);
        start_ping(cap, 2048);
        start_ping(cap, 4096);
        start_ping(cap, 8192);
        start_ping(cap, 16384);
        
        return 0;
}

static void start_ping(int cap, int size)
{
        int i = 0;
        struct timeval tv_start, tv_end;
	char mock_payload[MAX_PAYLOAD_SIZE];

        gettimeofday(&tv_start, NULL);
        for (i = 0; i < TEST_TIMES; ++i) {
                int label;
                int ret = vbinder_send(cap, mock_payload, size);
                vbinder_recv(&label, mock_payload, sizeof(mock_payload));
        }
        gettimeofday(&tv_end, NULL);
        LOGW("Bench-vbinder (%d times, sz=%d): %ld ms\n", TEST_TIMES, size,
               (tv_end.tv_sec - tv_start.tv_sec) * 1000 +
               (tv_end.tv_usec - tv_start.tv_usec) / 1000);
}

static void *pong_thread(void *dummy)
{
        int label;
        int cap = vbinder_create_channel(CHANNEL_LABEL, 1);
        pong_thr_tid = gettid();
        int ping_cap = vbinder_acquire_channel(ping_thr_tid, CHANNEL_LABEL);
        
        // printf("[%x] create_channel: cap=%d, errno=%d\n", gettid(), cap, errno);
        // fflush(stdout);
        char buf[MAX_PAYLOAD_SIZE];

        for (;;) {
                int ret = vbinder_recv(&label, buf, sizeof(buf));
                vbinder_send(ping_cap, buf, ret);
        }
        
        return NULL;
}
