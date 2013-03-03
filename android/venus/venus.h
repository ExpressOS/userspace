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

#ifndef VENUS_H_
#define VENUS_H_

#include <expressos/venus.h>

extern int g_binder_fd;
extern char *g_binder_vm_start;
extern int g_glue_fd;

/*
 * The version of Android Binder IPC supported by venus
 */
static const int VENUS_BINDER_VERSION = 7;

/*
 * Maximum number of threads supported
 */
static const int VENUS_BINDER_THREADS_NUM = 15;

static const int IPC_HELPER_NUM = 1;

/* Copy from android */
static const int BINDER_VM_SIZE = 4 * 1024 * 1024 - 4096;

int init_binder_ipc(void);
int init_glue(void);
int init_workers(void);
int dispatcher_loop(void);

void init_downcall_hdr(struct expressos_venus_downcall *d,
                       int type, size_t payload_size);
int send_downcall(const struct expressos_venus_downcall *arg);

int handle_close(const struct expressos_venus_upcall *);
int handle_pipe(const struct expressos_venus_upcall *);
int handle_socket(const struct expressos_venus_upcall *);
int handle_poll(const struct expressos_venus_upcall *);

int handle_binder_write_read(const struct expressos_venus_upcall *);
int handle_alien_mmap2(const struct expressos_venus_upcall *);
int handle_futex_wait(const struct expressos_venus_upcall *);
int handle_futex_wake(const struct expressos_venus_upcall *);

#endif
