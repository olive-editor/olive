

#ifndef _ofxsImageBlender_h_
#define _ofxsImageBlender_h_

#include "ofxsProcessing.H"

namespace OFX {

    /** @brief  Base class used to blend two images together */
    class ImageBlenderBase : public OFX::ImageProcessor {
    protected :
        const OFX::Image *_fromImg; // be this at 0
        const OFX::Image *_toImg;   // be this at 1
        float             _blend;   // how much to blend

    public :
        /** @brief no arg ctor */
        ImageBlenderBase(OFX::ImageEffect &instance)
          : OFX::ImageProcessor(instance)
          , _fromImg(0)
          , _toImg(0)
          , _blend(0.5f)
        {        
        }

        /** @brief set the src image */
        void setFromImg(const OFX::Image *v) {_fromImg = v;}
        void setToImg(const OFX::Image *v)   {_toImg = v;}

        /** @brief set the scale */
        void setBlend(float v) {_blend = v;}    
    };

    /** @brief templated class to blend between two images */
    template <class PIX, int nComponents>
    class ImageBlender : public ImageBlenderBase {
    public :
        // ctor
        ImageBlender(OFX::ImageEffect &instance) 
          : ImageBlenderBase(instance)
        {}

        static PIX Lerp(const PIX &v1,
                        const PIX &v2,
                        float blend)
        {
            return PIX((v2 - v1) * blend + v1);
        }

        // and do some processing
        void multiThreadProcessImages(OfxRectI procWindow)
        {
            float blend = _blend;
            float blendComp = 1.0f - blend;

            for(int y = procWindow.y1; y < procWindow.y2; y++) {
                if(_effect.abort()) break;

                PIX *dstPix = (PIX *) _dstImg->getPixelAddress(procWindow.x1, y);

                for(int x = procWindow.x1; x < procWindow.x2; x++) {
        
                    PIX *fromPix = (PIX *)  (_fromImg ? _fromImg->getPixelAddress(x, y) : 0);
                    PIX *toPix   = (PIX *)  (_toImg   ? _toImg->getPixelAddress(x, y)   : 0);
        
                    if(fromPix && toPix) {
                        for(int c = 0; c < nComponents; c++) 
                            dstPix[c] = Lerp(fromPix[c], toPix[c], blend);
                    }
                    else if(fromPix) {
                        for(int c = 0; c < nComponents; c++) 
                            dstPix[c] = PIX(fromPix[c] * blendComp);
                    }
                    else if(toPix) {
                        for(int c = 0; c < nComponents; c++) 
                            dstPix[c] = PIX(toPix[c] * blend);
                    }
                    else {
                        for(int c = 0; c < nComponents; c++) 
                            dstPix[c] = PIX(0);
                    }
                    
                    dstPix += nComponents;
                }
            }
        }
    
    };

};

#endif


