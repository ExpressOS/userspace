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

#include <sys/mman.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

#include <linux/binder.h>

int g_binder_fd = 0;
int g_glue_fd = 0;
char *g_binder_vm_start = NULL;

int init_binder_ipc(void)
{
        g_binder_fd = open("/dev/binder", O_RDWR);
        if (g_binder_fd < 0)
                return -1;

        g_binder_vm_start = mmap(NULL, BINDER_VM_SIZE, PROT_READ,
                                 MAP_PRIVATE | MAP_NORESERVE, g_binder_fd, 0);
        if (!g_binder_vm_start)
                return -ENOMEM;

        fcntl(g_binder_fd, F_SETFD, FD_CLOEXEC);

        int vers = 0;
        int result = ioctl(g_binder_fd, BINDER_VERSION, &vers);

        if (vers != VENUS_BINDER_VERSION)
                return -EINVAL;

        size_t max_threads = VENUS_BINDER_THREADS_NUM;
        result = ioctl(g_binder_fd, BINDER_SET_MAX_THREADS, &max_threads);
        if (result < 0)
                return result;

        return 0;
}

static int get_workspace(int *fd, unsigned *size)
{
        char *env = getenv("ANDROID_PROPERTY_WORKSPACE");
        if (!env)
                return -1;

        *fd = atoi(env);
        env = strchr(env, ',');
        if (!env)
                return -1;

        *size = atoi(env + 1);
        return 0;
}

int init_glue(void)
{
        g_glue_fd = open("/dev/expressos", O_RDWR);
        if (g_glue_fd < 0)
                return -1;

        struct expressos_venus_downcall ret;
        init_downcall_hdr(&ret, EXPRESSOS_VENUS_REGISTER_HELPER,
                          sizeof(struct expressos_venus_register_helper_out));

        struct expressos_venus_register_helper_out *r = &ret.register_helper;
        r->binder_vm_start = (unsigned long)g_binder_vm_start;
        get_workspace(&r->workspace_fd, &r->workspace_size);
        return send_downcall(&ret);
}
