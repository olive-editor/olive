

#ifndef _ofxsProcessing_h_
#define _ofxsProcessing_h_

/*
  OFX Support Library, a library that skins the OFX plug-in API with C++ classes.
  Copyright (C) 2005 The Open Effects Association Ltd
  Author Bruno Nicoletti bruno@thefoundry.co.uk

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name The Open Effects Association Ltd, nor the names of its
      contributors may be used to endorse or promote products derived from this
      software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The Open Effects Association Ltd
1 Wardour St
London W1D 6PA
England

*/

#include <cassert>
#include <algorithm>

#include "ofxsImageEffect.h"
#include "ofxsMultiThread.h"
#include "ofxsLog.h"

/** @file This file contains a useful base class that can be used to process images

The code below is not so much a skin on the base OFX classes, but code used in implementing
specific image processing algorithms.
*/

namespace OFX {

    ////////////////////////////////////////////////////////////////////////////////
    // base class to process images with
    class ImageProcessor : public OFX::MultiThread::Processor {
    protected :
        OFX::ImageEffect &_effect;      /**< @brief effect to render with */
        OFX::Image       *_dstImg;        /**< @brief image to process into */
        OfxRectI          _renderWindow;  /**< @brief render window to use */
        bool             _isEnabledOpenCLRender; /**< @brief is OpenCL Render Enabled */
        bool             _isEnabledCudaRender;   /**< @brief is Cuda Render Enabled */
        bool             _isEnabledMetalRender;   /**< @brief is Metal Render Enabled */
        void*            _pOpenCLCmdQ;           /**< @brief OpenCL Command Queue Handle */
        void*            _pCudaStream;           /**< @brief Cuda Stream Handle */
        void*            _pMetalCmdQ;           /**< @brief Metal Command Queue Handle */

    public :
        /** @brief ctor */
        ImageProcessor(OFX::ImageEffect &effect)
          : _effect(effect)
          , _dstImg(0)
          , _isEnabledOpenCLRender(false)
          , _isEnabledCudaRender(false)
          , _isEnabledMetalRender(false)
          , _pOpenCLCmdQ(NULL)
          , _pCudaStream(NULL)
          , _pMetalCmdQ(NULL)
        {
            _renderWindow.x1 = _renderWindow.y1 = _renderWindow.x2 = _renderWindow.y2 = 0;
        }

        /** @brief set the destination image */
        void setDstImg(OFX::Image *v) {_dstImg = v; }

        /** @brief set OpenCL, CUDA render arguments */
        void setGPURenderArgs(const OFX::RenderArguments& args)
        {
            _isEnabledOpenCLRender = args.isEnabledOpenCLRender;
            _isEnabledCudaRender = args.isEnabledCudaRender;
            _isEnabledMetalRender = args.isEnabledMetalRender;

            if (_isEnabledOpenCLRender)
            {
                _pOpenCLCmdQ = args.pOpenCLCmdQ;
            }
            if (_isEnabledCudaRender)
            {
                _pCudaStream = args.pCudaStream;
            }
            if (_isEnabledMetalRender)
            {
                _pMetalCmdQ = args.pMetalCmdQ;
            }
        }

        /** @brief reset the render window */
        void setRenderWindow(OfxRectI rect) {_renderWindow = rect;}

        /** @brief overridden from OFX::MultiThread::Processor. This function is called once on each SMP thread by the base class */
        void multiThreadFunction(unsigned int threadId, unsigned int nThreads)
        {
            // slice the y range into the number of threads it has
            unsigned int dy = _renderWindow.y2 - _renderWindow.y1;
            // the following is equivalent to std::ceil(dy/(double)nThreads);
            unsigned int h = (dy+nThreads-1)/nThreads;
            if (h == 0) {
                // there are more threads than lines to process
                h = 1;
            }
            if (threadId * h >= dy) {
                // empty render subwindow
                return;
            }
            unsigned int y1 = _renderWindow.y1 + threadId * h;

            unsigned int step = (threadId + 1) * h;
            unsigned int y2 = _renderWindow.y1 + (step < dy ? step : dy);

            OfxRectI win = _renderWindow;
            win.y1 = y1; win.y2 = y2;

            // and render that thread on each
            multiThreadProcessImages(win);
        }

        /** @brief called before any MP is done */
        virtual void preProcess(void) {}

        /** @brief this is called by process to actually process images using OpenCL when isEnabledOpenCLRender is true, override in derived classes */
        virtual void processImagesOpenCL(void)
        {
            OFX::Log::print("processImagesOpenCL not implemented");
            OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
        };

        /** @brief this is called by process to actually process images using CUDA when isEnabledCudaRender is true, override in derived classes */
        virtual void processImagesCuda(void)
        {
            OFX::Log::print("processImagesCuda not implemented");
            OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
        };

        /** @brief this is called by process to actually process images using Metal when isEnabledMetalRender is true, override in derived classes */
        virtual void processImagesMetal(void)
        {
            OFX::Log::print("processImagesMetal not implemented");
            OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
        };

        /** @brief this is called by multiThreadFunction to actually process images, override in derived classes */
        virtual void multiThreadProcessImages(OfxRectI window)
        {
            OFX::Log::print("multiThreadProcessImages not implemented");
            OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
        };

        /** @brief called before any MP is done */
        virtual void postProcess(void) {}

        /** @brief called to process everything */
        virtual void process(void)
        {
            // If _dstImg was set, check that the _renderWindow is lying into dstBounds
            if (_dstImg) {
                const OfxRectI& dstBounds = _dstImg->getBounds();
                // is the renderWindow within dstBounds ?
                assert(dstBounds.x1 <= _renderWindow.x1 && _renderWindow.x2 <= dstBounds.x2 &&
                       dstBounds.y1 <= _renderWindow.y1 && _renderWindow.y2 <= dstBounds.y2);
                // exit gracefully in case of error
                if (!(dstBounds.x1 <= _renderWindow.x1 && _renderWindow.x2 <= dstBounds.x2 &&
                      dstBounds.y1 <= _renderWindow.y1 && _renderWindow.y2 <= dstBounds.y2) ||
                    (_renderWindow.x1 >= _renderWindow.x2) ||
                    (_renderWindow.y1 >= _renderWindow.y2)) {
                    return;
                }
            }

            // call the pre MP pass
            preProcess();

            if (_isEnabledOpenCLRender)
            {
              OFX::Log::print("processing via OpenCL");
                processImagesOpenCL();
            }
            else if (_isEnabledCudaRender)
            {
              OFX::Log::print("processing via CUDA");
                processImagesCuda();
            }
            else if (_isEnabledMetalRender)
            {
              OFX::Log::print("processing via Metal");
                processImagesMetal();
            }
            else // is CPU
            {
              OFX::Log::print("processing via CPU");
                // make sure there are at least 4096 pixels per CPU and at least 1 line par CPU
                unsigned int nCPUs = (std::min(_renderWindow.x2 - _renderWindow.x1, 4096) *
                                      (_renderWindow.y2 - _renderWindow.y1)) / 4096;
                // make sure the number of CPUs is valid (and use at least 1 CPU)
                nCPUs = std::max(1u, std::min(nCPUs, OFX::MultiThread::getNumCPUs()));

                // call the base multi threading code, should put a pre & post thread calls in too
                multiThread(nCPUs);
            }

            // call the post MP pass
            postProcess();
        }

    };


};
#endif
