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

#define FLAGS_HAS_TIMEOUT 0x4

int sys_futex(int *uaddr, int op, int val, const struct timespec *timeout,
              int *uaddr2, int val3);

int handle_futex_wait(const struct expressos_venus_upcall *u)
{
        const struct expressos_venus_futex_wait_in *p = &u->async.futex_wait;
        const struct timespec *p_ts = p->op & FLAGS_HAS_TIMEOUT ? &p->ts : NULL;

        int retval = sys_futex((int*)p->uaddr, p->op, p->val, p_ts, NULL, p->bitset);

        struct expressos_venus_downcall ret;
        init_downcall_hdr(&ret, EXPRESSOS_VENUS_FUTEX_WAIT,
                          sizeof(u->async.handle) + sizeof(long));

        ret.async.handle = u->async.handle;
        ret.async.ret    = retval;

        return send_downcall(&ret);
}

int handle_futex_wake(const struct expressos_venus_upcall *u)
{
        const struct expressos_venus_futex_wake_in *p = &u->async.futex_wake;
        int retval = sys_futex((int*)p->uaddr, p->op, 0, NULL, NULL, p->bitset);

        struct expressos_venus_downcall ret;
        init_downcall_hdr(&ret, EXPRESSOS_VENUS_FUTEX_WAIT,
                          sizeof(u->async.handle) + sizeof(long));

        ret.async.handle = u->async.handle;
        ret.async.ret    = retval;

        return send_downcall(&ret);
}
