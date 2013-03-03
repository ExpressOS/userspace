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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include <pthread.h>
#include <linux/binder.h>

#define MAX_SEGMENT_NUM    16
#define READ_BUFFER_SIZE   (8 * 1024)
#define MARSHALER_BUF_SIZE (16 * 1024)

struct data_segment {
        unsigned offset;
        unsigned length;
};

struct ipc_marshaler {
        char                *buf;
        size_t              buf_size;
        char                *write_ptr;
        unsigned            data_entries;
        struct data_segment segments[MAX_SEGMENT_NUM];
};

static void patch_payload_ptr(char *buf, int entries, int offset);

static int marshaler_marshal(struct ipc_marshaler *this_,
                             const void *data_buffer,
                             size_t data_size);
static int marshal_transaction(struct ipc_marshaler *this_,
                               struct binder_transaction_data *tr);
static int record_segment(struct ipc_marshaler *this_,
				const void **ptr, size_t length);
static int append_data_segments(struct ipc_marshaler *this_);

/* #define DEBUG */
#ifdef DEBUG
static pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
static inline void dump_memory(const void *loc, size_t size);
#endif

int handle_binder_write_read(const struct expressos_venus_upcall *u)
{
        char buf[READ_BUFFER_SIZE];
        char marshaled_buffer[MARSHALER_BUF_SIZE];
        struct expressos_venus_downcall *d = NULL;
        size_t marshaled_read_buffer_size = 0;
        size_t payload_size, downcall_size;
        const struct expressos_venus_binder_write_read_in *b = &u->async.bwr;
        int ret;

        struct ipc_marshaler m = {
                .buf       = marshaled_buffer,
                .buf_size  = MARSHALER_BUF_SIZE,
        };

        struct binder_write_read bwr = {
                .write_buffer   = (unsigned long)b->payload,
                .write_size     = b->bwr_write_size,
                .read_buffer    = (unsigned long)buf,
                .read_size      = sizeof(buf),
        };

        patch_payload_ptr((char *)b->payload,
                          b->patch_table_entry_num,
                          b->patch_table_offset);

#ifdef DEBUG
        pthread_mutex_lock(&print_mutex);
        printf("=== write_buffer, tid=%d ===\n", gettid());
        dump_memory((void*)bwr.write_buffer, bwr.write_size);
        printf("===\n");
        pthread_mutex_unlock(&print_mutex);
#endif

        if ((ret = ioctl(g_binder_fd, BINDER_WRITE_READ, &bwr)) < 0)
                goto out;

#ifdef DEBUG
        pthread_mutex_lock(&print_mutex);
        printf("=== read_buffer, tid=%d, ret=%d ===\n", gettid(), ret);
        dump_memory((void*)bwr.read_buffer, bwr.read_consumed);
        printf("===\n");
        pthread_mutex_unlock(&print_mutex);
#endif

        ret = marshaler_marshal(&m, (const void *)bwr.read_buffer,
                                bwr.read_consumed);

        marshaled_read_buffer_size = ret < 0 ? 0 : m.write_ptr - m.buf;

out:
        payload_size = sizeof(u->async.handle) +
                        sizeof(struct expressos_venus_binder_write_read_out) +
                        marshaled_read_buffer_size;

        downcall_size = sizeof(struct expressos_venus_hdr) + payload_size;

        d = malloc(downcall_size);
        init_downcall_hdr(d, EXPRESSOS_VENUS_BINDER_WRITE_READ,
                          payload_size);

        d->async.handle             = u->async.handle;
        d->async.bwr.buffer         = b->buffer;
        d->async.bwr.ret            = ret;
        d->async.bwr.write_consumed = bwr.write_consumed;
        d->async.bwr.read_consumed  = bwr.read_consumed;
        d->async.bwr.payload_size   = marshaled_read_buffer_size;
        d->async.bwr.data_entries   = m.data_entries;
        memcpy(d->async.bwr.payload, marshaled_buffer,
               marshaled_read_buffer_size);

        ret = send_downcall(d);

        free(d);
        return ret;
}

static void patch_payload_ptr(char *buf, int entries, int offset)
{
        int i = 0;
        int *patch_table = (int *)(buf + offset);

        for (i = 0; i < entries; ++i)
                *(int *)(buf + patch_table[i]) += (uintptr_t) buf;
}

/*
 * Marshal read buffer
 * The position of read buffer matters, thus the format is:
 * bwr_read_buffer
 * payload_entry_num
 * payload = <offset_in_binder_vm, length, data>
 */

static int
marshaler_marshal(struct ipc_marshaler *this_,
                  const void *bwr_read_buffer, size_t size)
{
        char *ptr, *end;

        this_->write_ptr = this_->buf;
        memcpy(this_->write_ptr, bwr_read_buffer, size);

        ptr = this_->write_ptr;
        this_->write_ptr += size;
        end = this_->write_ptr;

        while (ptr < end) {
                if (ptr + sizeof(unsigned) > end)
                        return -EFAULT;

                unsigned cmd = *(unsigned*)ptr;
                ptr += sizeof(unsigned);

                switch (cmd) {
                        case BR_NOOP:
                        case BR_TRANSACTION_COMPLETE:
                        case BR_SPAWN_LOOPER:
                                break;

                        case BR_TRANSACTION:
                        case BR_REPLY:
                                {
                                        if (ptr + sizeof(struct binder_transaction_data) > end)
                                                return -EFAULT;

                                        struct binder_transaction_data *tr = (struct binder_transaction_data*)ptr;
                                        ptr += sizeof(*tr);

                                        int ret = marshal_transaction(this_, tr);
                                        if (ret < 0)
                                                return ret;
                                        break;
                                }

                        case BR_INCREFS:
                        case BR_DECREFS:
                        case BR_ACQUIRE:
                        case BR_RELEASE:
                                {
                                        ptr += 2 * sizeof(void *);
                                        break;
                                }

                        default:
                                printf("marshaler_marshal: unsupported IPC primitive 0x%x\n",
                                       cmd);
                                return -1;
                }
        }

        int r = append_data_segments(this_);
        if (r != 0) {
                return -1;
        }

        return 0;
}

static int marshal_transaction(struct ipc_marshaler *this_,
                               struct binder_transaction_data *tr)
{
        int ret;
        if (tr->data_size &&
            (ret = record_segment
             (this_, &tr->data.ptr.buffer, tr->data_size)) < 0)
                return ret;

        if (tr->offsets_size &&
            (ret = record_segment
             (this_, &tr->data.ptr.offsets, tr->offsets_size)) < 0)
                return ret;

        return 0;
}

static int record_segment(struct ipc_marshaler *this_, const void **ptr,
                          size_t length)
{
        if (this_->data_entries >= MAX_SEGMENT_NUM)
                return -ENOMEM;

        const char *data_ptr = (const char*)*ptr;

        if (data_ptr < g_binder_vm_start
            || (data_ptr >= g_binder_vm_start + BINDER_VM_SIZE))
                return -EFAULT;

        struct data_segment e = {
                .offset = data_ptr - g_binder_vm_start,
                .length = length,
        };

        this_->segments[this_->data_entries++] = e;
        *(unsigned *)ptr = e.offset;

        return 0;
}

static int append_data_segments(struct ipc_marshaler *this_)
{
        const struct data_segment *e   = this_->segments;
        const struct data_segment *end = e + this_->data_entries;

        while (e < end) {
                if ((size_t)(this_->buf + this_->buf_size - this_->write_ptr) <
                    sizeof(struct data_segment) + e->length)
                        return -ENOMEM;

                memcpy(this_->write_ptr, e, sizeof(struct data_segment));
                this_->write_ptr += sizeof(struct data_segment);

                memcpy(this_->write_ptr, g_binder_vm_start + e->offset,
                       e->length);
                this_->write_ptr += e->length;

                ++e;
        }
        return 0;
}

#ifdef DEBUG
static inline void dump_memory(const void *loc, size_t size)
{
        size_t i;
        for (i = 1; i <= (size + 3) / 4; ++i) {
                printf("0x%08x ", ((const unsigned int *) loc)[i - 1]);
                if (i % 4 == 0)
                        printf("\n");
        }
        printf("\n");
}
#endif
