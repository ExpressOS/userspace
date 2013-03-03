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
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>

int main()
{
        int i = 0;
        int is_main = 1;
        int ret;

        // fork ipc helper
        for (i = 1; i < IPC_HELPER_NUM; ++i) {
                pid_t pid = fork();
                if (pid == 0) {
                        // child
                        is_main = 0;
                } else if (pid < 0) {
                        fprintf(stderr, "Failed to fork IPC helper, exiting\n");
                        return -1;
                }
        }

        if ((ret = init_binder_ipc())) {
                fprintf(stderr, "Cannot intialize binder IPC channel, ret=%d\n", ret);
                return 1;
        }

        if ((ret = init_glue())) {
                fprintf(stderr, "Cannot initialize glue, ret=%d\n", ret);
                return 1;
        }

        if ((ret = init_workers())) {
                fprintf(stderr, "Cannot initialize worker threads, ret=%d\n", ret);
                return 1;
        }

        printf("[%d] Venus ready\n", getpid());

        dispatcher_loop();

        printf("Venus exited.\n");
        return 0;
}
