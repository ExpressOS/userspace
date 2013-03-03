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

#include <ui/DisplayInfo.h>
#include <ui/EGLUtils.h>

#include <core/SkBitmap.h>
#include <images/SkImageDecoder.h>

#include <EGL/egl.h>
#include <GLES/glext.h>
#include <EGL/eglext.h>

#include <utils/Log.h>

namespace
{
template<class T>
class auto_array_ptr
{
    public:
        auto_array_ptr(T *ptr)
                        : ptr_(ptr)
        {}
        ~auto_array_ptr() { delete [] ptr_; }
        T *get() const { return ptr_; }
    private:
        auto_array_ptr(const auto_array_ptr &);
        auto_array_ptr &operator=(const auto_array_ptr &);
        T *ptr_;
};
}

using namespace android;

Presenter::Presenter()
  : Thread(false)
  , mSession(new SurfaceComposerClient())
  , mCurrentSlide(0)
  , mNumberOfSlides(0)
  , mSlideWidth(0)
  , mSlideHeight(0)
  , mBlankScreen(0)
  , mAction(NO_OP)
{
        pthread_cond_init(&mCond, NULL);
        pthread_mutex_init(&mCondMutex, NULL);
}

Presenter::~Presenter()
{
        pthread_mutex_destroy(&mCondMutex);
        pthread_cond_destroy(&mCond);
}

void Presenter::signalAction(Action action)
{
        if (action == NO_OP)
                return;
        
        pthread_mutex_lock(&mCondMutex);
        while (mAction != NO_OP)
                pthread_cond_wait(&mCond, &mCondMutex);
        
        mAction = action;
        pthread_cond_signal(&mCond);
        pthread_mutex_unlock(&mCondMutex);
}

int Presenter::open(const char *zipArchive, void *(*handler)(void*))
{
        mHandler = handler;
        
        status_t r;
        if ((r = mSlides.open(zipArchive)))
                return r;

        ZipEntryRO desc_entry = mSlides.findEntryByName("desc.txt");
        if (!desc_entry)
                return -EACCES;

        size_t uncompressed_length;
        if (!mSlides.getEntryInfo(desc_entry, NULL, &uncompressed_length,
                                  NULL, NULL, NULL, NULL))
                return -EINVAL;

        auto_array_ptr<char> buffer(new char[uncompressed_length]);
        
        if (!mSlides.uncompressEntry(desc_entry, buffer.get()))
                return -EINVAL;

        if ((sscanf(buffer.get(), "%u %u %u", &mNumberOfSlides, &mSlideWidth, &mSlideHeight) != 3))
                return -EINVAL;

        pthread_t thread;
        pthread_create(&thread, NULL, mHandler, NULL);
        
        return 0;
}

int Presenter::loadSlide(size_t n)
{
        char filename[256];
        snprintf(filename, sizeof(filename), "%u.png", n);

        ZipEntryRO entry = mSlides.findEntryByName(filename);
        if (!entry)
                return -EACCES;

        size_t uncompressed_length;
        if (!mSlides.getEntryInfo(entry, NULL, &uncompressed_length,
                                  NULL, NULL, NULL, NULL))
                return -EINVAL;

        auto_array_ptr<char> buffer(new char[uncompressed_length]);

        if (!mSlides.uncompressEntry(entry, buffer.get()))
                return -EINVAL;

        SkBitmap bitmap;

        SkImageDecoder::DecodeMemory(buffer.get(), uncompressed_length,
                                     &bitmap, SkBitmap::kRGB_565_Config,
                                     SkImageDecoder::kDecodePixels_Mode);

        // ensure we can call getPixels(). No need to call unlock, since the
        // bitmap will go out of scope when we return from this method.
        bitmap.lockPixels();

        const int w = bitmap.width();
        const int h = bitmap.height();
        const void* p = bitmap.getPixels();
       
        GLint crop[4] = { 0, h, w, -h };
        int tw = 1 << (31 - __builtin_clz(w));
        int th = 1 << (31 - __builtin_clz(h));
        if (tw < w) tw <<= 1;
        if (th < h) th <<= 1;

        switch (bitmap.getConfig()) {
                case SkBitmap::kARGB_8888_Config:
                        if (tw != w || th != h) {
                                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tw, th, 0, GL_RGBA,
                                             GL_UNSIGNED_BYTE, 0);
                                glTexSubImage2D(GL_TEXTURE_2D, 0,
                                                0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, p);
                        } else {
                                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tw, th, 0, GL_RGBA,
                                             GL_UNSIGNED_BYTE, p);
                        }
                        break;

                case SkBitmap::kRGB_565_Config:
                        if (tw != w || th != h) {
                                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tw, th, 0, GL_RGB,
                                             GL_UNSIGNED_SHORT_5_6_5, 0);
                                glTexSubImage2D(GL_TEXTURE_2D, 0,
                                                0, 0, w, h, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, p);
                        } else {
                                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tw, th, 0, GL_RGB,
                                             GL_UNSIGNED_SHORT_5_6_5, p);
                        }
                        break;
                default:
                        LOGE("Unknown file format\n");
                        break;
        }

        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES, crop);

        bitmap.unlockPixels();
        mCurrentSlide = n;
        return NO_ERROR;
}

void Presenter::onFirstRef() {
#if 0
    status_t err = mSession->linkToComposerDeath(this);
    printf("onFirstRef: err=%d, tid=%d\n", err, gettid());
    fflush(stdout);
    LOGE_IF(err, "linkToComposerDeath failed (%s) ", strerror(-err));
    if (err == NO_ERROR) {
//        run("BootAnimation", PRIORITY_DISPLAY);
    }
#endif
    run("Presenter", PRIORITY_DISPLAY);
//    threadLoop();
}

status_t Presenter::readyToRun()
{
        DisplayInfo dinfo;
        status_t status = session()->getDisplayInfo(0, &dinfo);
        if (status)
                return -1;

        // create the native surface
        sp<SurfaceControl> control = session()->createSurface(
            getpid(), 0, dinfo.w, dinfo.h, PIXEL_FORMAT_RGB_565);
        session()->openTransaction();
        control->setLayer(0x40000000);
        session()->closeTransaction();

        sp<Surface> s = control->getSurface();

        // initialize opengl and egl
        const EGLint attribs[] = {
                EGL_DEPTH_SIZE, 0,
                EGL_NONE
        };
        EGLint w, h, dummy;
        EGLint numConfigs;
        EGLConfig config;
        EGLSurface surface;
        EGLContext context;

        EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

        eglInitialize(display, 0, 0);
        EGLUtils::selectConfigForNativeWindow(display, attribs, s.get(), &config);
        surface = eglCreateWindowSurface(display, config, s.get(), NULL);
        context = eglCreateContext(display, config, NULL, NULL);
        eglQuerySurface(display, surface, EGL_WIDTH, &w);
        eglQuerySurface(display, surface, EGL_HEIGHT, &h);

        if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
                return NO_INIT;

        mDisplay = display;
        mContext = context;
        mSurface = surface;
        mWidth = w;
        mHeight = h;
        mFlingerSurfaceControl = control;
        mFlingerSurface = s;

        LOGI ("Screen initialized. w=%d, h=%d\n", w, h);
        return NO_ERROR;
}

bool Presenter::threadLoop()
{
    bool r;
    // clear screen
    glShadeModel(GL_FLAT);
    glDisable(GL_DITHER);
    glDisable(GL_SCISSOR_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    eglSwapBuffers(mDisplay, mSurface);

    //    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    //    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glShadeModel(GL_FLAT);

    glEnable(GL_TEXTURE_2D);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    glTexEnvx(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    // Blend state
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    r = runPresentation();
    // eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    // eglDestroyContext(mDisplay, mContext);
    // eglDestroySurface(mDisplay, mSurface);
    // mFlingerSurface.clear();
    // mFlingerSurfaceControl.clear();
    // eglTerminate(mDisplay);
    // IPCThreadState::self()->stopProcess();
    return r;
}

void Presenter::display()
{
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (!mBlankScreen)
        {
                glDrawTexiOES(0, 0, 0, mSlideWidth, mSlideHeight);
        }
    
        EGLBoolean res = eglSwapBuffers(mDisplay, mSurface);
        if (res == EGL_FALSE)
                printf ("Failed to update display\n");
}

bool Presenter::runPresentation()
{
        int r = loadSlide(1);

        display();

        while (true)
        {
                pthread_mutex_lock(&mCondMutex);
                while (mAction == NO_OP)
                        pthread_cond_wait(&mCond, &mCondMutex);
                
                switch (mAction)
                {
                        case PREV_SLIDE:
                                if (mCurrentSlide > 1)
                                {
                                        loadSlide(--mCurrentSlide);
                                        display();
                                }
                                break;

                        case NEXT_SLIDE:
                                if (mCurrentSlide < mNumberOfSlides)
                                {
                                        loadSlide(++mCurrentSlide);
                                        display();
                                }
                                break;
        
                        case TOGGLE_BLANK:
                                mBlankScreen = !mBlankScreen;
                                display();
                                break;
                                
                        default:
                                printf("Unknown action\n");
                                break;
                }
                
                mAction = NO_OP;
                pthread_mutex_unlock(&mCondMutex);
        }
        return false;
}
