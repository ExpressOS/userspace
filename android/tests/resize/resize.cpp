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

#include <cutils/memory.h>

#include <utils/Log.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>

#include <surfaceflinger/Surface.h>
#include <surfaceflinger/ISurface.h>
#include <surfaceflinger/SurfaceComposerClient.h>

#include <ui/Overlay.h>

using namespace android;

int main(int argc, char** argv)
{
    // set up the thread-pool
    sp<ProcessState> proc(ProcessState::self());
    ProcessState::self()->startThreadPool();

    // create a client to surfaceflinger
    sp<SurfaceComposerClient> client = new SurfaceComposerClient();

    // create pushbuffer surface
    sp<SurfaceControl> control = client->createSurface(getpid(), 0, 160, 240, 
            PIXEL_FORMAT_RGB_565);
    printf("client.initCheck() = %x, control=%p\n", client->initCheck(), control.get());
    fflush(stdout);

    printf("display width=%ld, display heigth=%ld\n", client->getDisplayWidth(0), client->getDisplayHeight(0));
    sp<Surface> surface = control->getSurface();
    fflush(stdout);

    client->openTransaction();
    control->setLayer(100000);
    client->closeTransaction();

    Surface::SurfaceInfo info;
    surface->lock(&info);
    ssize_t bpr = info.s * bytesPerPixel(info.format);
    android_memset16((uint16_t*)info.bits, 0xF800, bpr*info.h);
    surface->unlockAndPost();

    surface->lock(&info);
    android_memset16((uint16_t*)info.bits, 0x07E0, bpr*info.h);
    surface->unlockAndPost();

    client->openTransaction();
    control->setSize(320, 240);
    client->closeTransaction();

    
    IPCThreadState::self()->joinThreadPool();
    
    return 0;
}
