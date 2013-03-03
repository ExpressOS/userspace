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

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <poll.h>

int handle_socket(const struct expressos_venus_upcall *u)
{
        const struct expressos_venus_socket_in *p = &u->async.socket;

        int retval = socket(p->domain, p->type, p->protocol);
        if (retval < 0)
                retval = -errno;

        struct expressos_venus_downcall ret;
        init_downcall_hdr(&ret, EXPRESSOS_VENUS_SOCKET,
                          sizeof(u->async.handle) + sizeof(long));

        ret.async.handle = u->async.handle;
        ret.async.ret    = retval;

        return send_downcall(&ret);
}

int handle_poll(const struct expressos_venus_upcall *u)
{
        const struct expressos_venus_poll_in *p = &u->async.poll;
        size_t pollfd_size = p->nfds * sizeof(struct pollfd);
        size_t payload_size = sizeof(u->async.handle) + pollfd_size
                        + sizeof(struct expressos_venus_poll_out);
        size_t downcall_size = sizeof(struct expressos_venus_hdr) + payload_size;

        struct expressos_venus_downcall *ret = malloc(downcall_size);
        if (!ret)
                return -ENOMEM;

        init_downcall_hdr(ret, EXPRESSOS_VENUS_POLL, payload_size);
        struct expressos_venus_poll_out *res = &ret->async.poll;
        memcpy(res->fds, p->fds, pollfd_size);

        int retval = poll((struct pollfd*)res->fds,
                          p->nfds, p->timeout);
        if (retval < 0)
                retval = -errno;

        ret->async.handle  = u->async.handle;
        res->buffer        = p->buffer;
        res->ret           = retval;

        int r = send_downcall(ret);
        free(ret);
        return r;
}
