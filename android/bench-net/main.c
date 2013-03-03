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

#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

#define LOG_TAG "bench-nc"

#include <cutils/log.h>

static void panic(const char * str)
{
        puts(str);
	printf("errno=%d\n", errno);
        fflush(stdout);
        exit(1);
}

int main(int argc, const char *argv[])
{
        struct sockaddr_in serv_addr;
	const char *addr_str = argv[1];
	int port = strtol(argv[2], NULL, 10);
        if (inet_aton(addr_str, &serv_addr.sin_addr) < 1)
                panic("aton failed");

        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
                panic("socket() failed");

        serv_addr.sin_port = htons(port);
        serv_addr.sin_family = AF_INET;

        if (connect(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
                printf("cannot connect to ip=%s:%d\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
                panic("ERROR connecting");
        }

        char buf[8192];
        int transmitted = 0;
        struct timeval tv_start, tv_end;

        gettimeofday(&tv_start, NULL);
        for (;;) {
                int r = read(sockfd, buf, sizeof(buf));
                if (r <= 0)
                        break;
                transmitted += r;
        }
        gettimeofday(&tv_end, NULL);

        LOGI("transmitted: %d K time: %ld ms\n", transmitted / 1024,
             (tv_end.tv_sec - tv_start.tv_sec) * 1000 +
             (tv_end.tv_usec - tv_start.tv_usec) / 1000 );
        
        close(sockfd);
        return 0;
}

