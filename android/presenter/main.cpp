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

#define LOG_TAG "Presenter"

#include "presenter.h"

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>

#include <utils/Log.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/poll.h>

static const unsigned short s_port = 9930;

using namespace android;

static sp<Presenter> s_presenter;

static void *key_handler(void*)
{
        int s, s2;
        char str[100];
        struct sockaddr_in local, remote;
        if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        {
                printf ("Cannot open socket\n");
                exit(1);
        }

        memset(&local, 0, sizeof(local));
        local.sin_family = AF_INET;
        local.sin_port = htons(s_port);
        local.sin_addr.s_addr = htonl(INADDR_ANY);

        
        if (bind(s, (sockaddr*)&local, sizeof(local))==-1)
        {
                printf ("Failed to bind\n");
                exit(1);
        }

        int rlen;

        struct pollfd cmd_fd;
        cmd_fd.fd = s;
        cmd_fd.events = POLLIN;
        cmd_fd.revents = 0;
        
        printf("Started\n");
        char c;
        while (poll(&cmd_fd, 1, -1) == 1)
        {
                recvfrom(s, &c, 1, 0, (sockaddr*)&remote, &rlen);
                switch (c)
                {
                        case 'N':
                        case 'n':
                                s_presenter->signalAction(Presenter::NEXT_SLIDE);
                                break;

                        case 'P':
                        case 'p':
                                s_presenter->signalAction(Presenter::PREV_SLIDE);
                                break;

                        case 'b':
                        case 'B':
                                s_presenter->signalAction(Presenter::TOGGLE_BLANK);
                                break;
                }
        }
        printf("End\n");
        return NULL;
}

int main(int argc, char **argp)
{
        const char *filename = getenv("SLIDES");

        sp<ProcessState> proc(ProcessState::self());
        ProcessState::self()->startThreadPool();

        s_presenter = new Presenter();

        int r = s_presenter->open(filename, key_handler);
        if (r)
        {
                LOGE("Cannot open file `%s`, r=%d\n", filename, r);
                return 1;
        }

        IPCThreadState::self()->joinThreadPool();
        return 0;
}
