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

#include "venus.h"

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

/*
 * Worker thread for venus. The main assumption that venus has is that
 * there is 1:1 mapping between a thread in ExpressOS and a thread in
 * venus. The reason behind this is that the Android Binder IPC
 * contains thread-level state.
 *
 * To enforce there is no deadlocks in the system, upcalls must be
 * mapped to these worker thread in a consistent way. There are two
 * types of calls, asynchronous and synchronous calls, both of which
 * are mapped to worker threads in a pre-defined way.
 */
struct worker_thread {
        pthread_t self;
        pthread_mutex_t mutex;
        pthread_cond_t  cond;
        int venus_fd;
        int id;
        struct expressos_venus_upcall *arg;
};

#define WORKER_NUM 20

/*
 * 12 for L4_CAP, an additional one because on we create an ipc_gate()
 * associated with the thread, so the cap is always either even or
 * odd.
 */
#define HANDLE_SHIFT 13

static struct worker_thread workers[WORKER_NUM];

static void *worker_loop(void *);
static void post_upcall(const struct expressos_venus_upcall *upcall);

int init_workers(void)
{
        int i, r;
        for (i = 0; i < WORKER_NUM; ++i) {
                struct worker_thread w = {
                        .mutex = PTHREAD_MUTEX_INITIALIZER,
                        .cond  = PTHREAD_COND_INITIALIZER,
                        .id    = i,
                };
                workers[i] = w;

                r = pthread_create(&workers[i].self, NULL, worker_loop, (void*)&workers[i]);
                if (r)
                        return r;
        }
        return 0;
}

int dispatcher_loop(void)
{
        static const size_t BUF_SIZE = 16384;
        char buf[BUF_SIZE];
        const char *ptr, *end;

        int r;
        for (;;) {
                struct expressos_venus_write_read wr = {
                        .read_size   = BUF_SIZE,
                        .read_buffer = (unsigned long)buf,
                };
                r = ioctl(g_glue_fd, EXPRESSOS_VENUS_WRITE_READ, &wr);
                if (r) {
                        fprintf(stderr, "ioctl error, ret=%d\n", r);
                        return r;
                }

                ptr = buf;
                end = buf + wr.read_consumed;
                while (ptr < end) {
                        const struct expressos_venus_upcall *upcall = (struct expressos_venus_upcall*)ptr;
                        size_t s = sizeof(struct expressos_venus_hdr) + upcall->hdr.payload_size;
                        if (ptr + s > end) {
                                fprintf(stderr, "Malformed message\n");
                                return -1;
                        }

                        post_upcall(upcall);
                        ptr += s;
                }
        }

        return 0;
}

void *worker_loop(void *t)
{
        struct worker_thread *w = (struct worker_thread*)t;
        struct expressos_venus_upcall *u;
        int retval = 0;

        for (;;) {
                pthread_mutex_lock(&w->mutex);
                while (!w->arg)
                        pthread_cond_wait(&w->cond, &w->mutex);

                u = w->arg;
                w->arg = NULL;
                pthread_mutex_unlock(&w->mutex);

                switch (u->hdr.opcode) {
                        case EXPRESSOS_VENUS_CLOSE:
                                retval = handle_close(u);
                                break;
                        case EXPRESSOS_VENUS_PIPE:
                                retval = handle_pipe(u);
                                break;
                        case EXPRESSOS_VENUS_SOCKET:
                                retval = handle_socket(u);
                                break;
                        case EXPRESSOS_VENUS_POLL:
                                retval = handle_poll(u);
                                break;
                        case EXPRESSOS_VENUS_BINDER_WRITE_READ:
                                retval = handle_binder_write_read(u);
                                break;
                        case EXPRESSOS_VENUS_ALIEN_MMAP2:
                                retval = handle_alien_mmap2(u);
                                break;
                        case EXPRESSOS_VENUS_FUTEX_WAIT:
                                retval = handle_futex_wait(u);
                                break;
                        case EXPRESSOS_VENUS_FUTEX_WAKE:
                                retval = handle_futex_wake(u);
                                break;
                        default:
                                fprintf(stderr, "venus: unknown upcall with opcode %d\n", u->hdr.opcode);
                                break;
                }

                if (retval) {
                        fprintf(stderr, "venus: error while handling upcall, opcode=%d, retval=%d\n", u->hdr.opcode, retval);
                        free(u);
                        break;
                }
                free(u);
        }

        return NULL;
}

static struct worker_thread *find_worker(const struct expressos_venus_upcall *upcall)
{
        unsigned o = upcall->hdr.opcode;
        if (o >= EXPRESSOS_VENUS_CALL_COUNT) {
                fprintf(stderr, "Unknown upcall with opcode %d\n", o);
                return NULL;
        }

        /*
         * The code assumes that the handle is the first field for all
         * asynchronous calls.
         */
        size_t worker_id;
        switch (o) {
                case EXPRESSOS_VENUS_SOCKET:
                case EXPRESSOS_VENUS_BINDER_WRITE_READ:
                case EXPRESSOS_VENUS_POLL:
                case EXPRESSOS_VENUS_FUTEX_WAIT:
                case EXPRESSOS_VENUS_FUTEX_WAKE:
                        worker_id = (upcall->async.handle >> HANDLE_SHIFT) % (WORKER_NUM - 1);
                        break;

                default:
                        worker_id = WORKER_NUM - 1;
        }

        return &workers[worker_id];
}

/*
 * Post an upcall to a worker thread. The function will create a copy
 * of the upcall and attach it to the worker thread.
 */
static void post_upcall(const struct expressos_venus_upcall *upcall)
{
        struct worker_thread *w = find_worker(upcall);
        if (!w)
                return;

        pthread_mutex_lock(&w->mutex);

        if (w->arg) {
                fprintf(stderr,
                        "[%d] Has suprious command, current_work_type=%d, "
                        "new_work_type=%d, new_work_handle=0x%08x\n",
                        w->id, w->arg->hdr.opcode,
                        upcall->hdr.opcode, upcall->async.handle);
        }

        size_t s = sizeof(struct expressos_venus_hdr) + upcall->hdr.payload_size;
        struct expressos_venus_upcall *arg = malloc(s);
        memcpy(arg, upcall, s);
        w->arg = arg;

        pthread_cond_signal(&w->cond);
        pthread_mutex_unlock(&w->mutex);
}

int send_downcall(const struct expressos_venus_downcall *arg)
{
        struct expressos_venus_write_read wr = {
                .write_size = sizeof(struct expressos_venus_hdr) + arg->hdr.payload_size,
                .write_buffer = (unsigned long)arg,
        };

        return ioctl(g_glue_fd, EXPRESSOS_VENUS_WRITE_READ, &wr);
}

void init_downcall_hdr(struct expressos_venus_downcall *d,
                       int type, size_t payload_size)
{
        d->hdr.opcode = type;
        d->hdr.payload_size = payload_size;
}
