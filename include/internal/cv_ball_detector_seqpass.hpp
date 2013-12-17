#ifndef TRIK_VIDTRANSCODE_CV_INTERNAL_CV_BALL_DETECTOR_SEQPASS_HPP_
#define TRIK_VIDTRANSCODE_CV_INTERNAL_CV_BALL_DETECTOR_SEQPASS_HPP_

#ifndef __cplusplus
#error C++-only header
#endif

#include <string.h>
#include <cassert>
#include <cmath>
#include <c6x.h>

#include "internal/stdcpp.hpp"
#include "trik_vidtranscode_cv.h"
#include "internal/cv_hsv_range_detector.hpp"


/* **** **** **** **** **** */ namespace trik /* **** **** **** **** **** */ {

/* **** **** **** **** **** */ namespace cv /* **** **** **** **** **** */ {



#warning Eliminate global var
static uint64_t s_rgb888hsv[640*480];
static int s_color[32][32][32];

union U_Hsv8x3
{
    struct {
      uint8_t h;
      uint8_t s;
      uint8_t v;
      uint8_t none; //unused
    } parts;
    uint32_t whole;
};

enum Enum_Color {BLACK, WHITE, GREY, RED, ORANGE, YELLOW, GREEN, AQUA, BLUE, PURPLE};

int img_width  = 320;
int img_height = 240;
int m_color[10];

template <>
class BallDetector<TRIK_VIDTRANSCODE_CV_VIDEO_FORMAT_YUV422, TRIK_VIDTRANSCODE_CV_VIDEO_FORMAT_RGB565X> : public CVAlgorithm
{
  private:
    uint64_t m_detectRange;
    uint32_t m_detectExpected;
    uint32_t m_srcToDstShift;

    int32_t  m_targetX;
    int32_t  m_targetY;
    uint32_t m_targetPoints;

    TrikCvImageDesc m_inImageDesc;
    TrikCvImageDesc m_outImageDesc;

    static uint16_t* restrict s_mult43_div;  // allocated from fast ram
    static uint16_t* restrict s_mult255_div; // allocated from fast ram

    static void __attribute__((always_inline)) writeOutputPixel(uint16_t* restrict _rgb565ptr,
                                                                const uint32_t _rgb888)
    {
      *_rgb565ptr = ((_rgb888>>19)&0x001f) | ((_rgb888>>5)&0x07e0) | ((_rgb888<<8)&0xf800);
    }

    void __attribute__((always_inline)) drawOutputPixelBound(const int32_t _srcCol,
                                                             const int32_t _srcRow,
                                                             const int32_t _srcColBot,
                                                             const int32_t _srcColTop,
                                                             const int32_t _srcRowBot,
                                                             const int32_t _srcRowTop,
                                                             const TrikCvImageBuffer& _outImage,
                                                             const uint32_t _rgb888) const
    {
      const int32_t srcCol = range<int32_t>(_srcColBot, _srcCol, _srcColTop);
      const int32_t srcRow = range<int32_t>(_srcRowBot, _srcRow, _srcRowTop);

      const int32_t dstRow = srcRow >> m_srcToDstShift;
      const int32_t dstCol = srcCol >> m_srcToDstShift;

      const uint32_t dstOfs = dstRow*m_outImageDesc.m_lineLength + dstCol*sizeof(uint16_t);
      writeOutputPixel(reinterpret_cast<uint16_t*>(_outImage.m_ptr+dstOfs), _rgb888);
    }

    void __attribute__((always_inline)) drawOutputCircle(const int32_t _srcCol,
                                                         const int32_t _srcRow,
                                                         const int32_t _srcRadius,
                                                         const TrikCvImageBuffer& _outImage,
                                                         const uint32_t _rgb888) const
    {
      const int32_t widthBot  = 0;
      const int32_t widthTop  = m_inImageDesc.m_width-1;
      const int32_t heightBot = 0;
      const int32_t heightTop = m_inImageDesc.m_height-1;

      int32_t circleError  = 1-_srcRadius;
      int32_t circleErrorY = 1;
      int32_t circleErrorX = -2*_srcRadius;
      int32_t circleX = _srcRadius;
      int32_t circleY = 0;

      drawOutputPixelBound(_srcCol, _srcRow+_srcRadius, widthBot, widthTop, heightBot, heightTop, _outImage, _rgb888);
      drawOutputPixelBound(_srcCol, _srcRow-_srcRadius, widthBot, widthTop, heightBot, heightTop, _outImage, _rgb888);
      drawOutputPixelBound(_srcCol+_srcRadius, _srcRow, widthBot, widthTop, heightBot, heightTop, _outImage, _rgb888);
      drawOutputPixelBound(_srcCol-_srcRadius, _srcRow, widthBot, widthTop, heightBot, heightTop, _outImage, _rgb888);

      while (circleY < circleX)
      {
        if (circleError >= 0)
        {
          circleX      -= 1;
          circleErrorX += 2;
          circleError  += circleErrorX;
        }
        circleY      += 1;
        circleErrorY += 2;
        circleError  += circleErrorY;

        drawOutputPixelBound(_srcCol+circleX, _srcRow+circleY, widthBot, widthTop, heightBot, heightTop, _outImage, _rgb888);
        drawOutputPixelBound(_srcCol+circleX, _srcRow-circleY, widthBot, widthTop, heightBot, heightTop, _outImage, _rgb888);
        drawOutputPixelBound(_srcCol-circleX, _srcRow+circleY, widthBot, widthTop, heightBot, heightTop, _outImage, _rgb888);
        drawOutputPixelBound(_srcCol-circleX, _srcRow-circleY, widthBot, widthTop, heightBot, heightTop, _outImage, _rgb888);
        drawOutputPixelBound(_srcCol+circleY, _srcRow+circleX, widthBot, widthTop, heightBot, heightTop, _outImage, _rgb888);
        drawOutputPixelBound(_srcCol+circleY, _srcRow-circleX, widthBot, widthTop, heightBot, heightTop, _outImage, _rgb888);
        drawOutputPixelBound(_srcCol-circleY, _srcRow+circleX, widthBot, widthTop, heightBot, heightTop, _outImage, _rgb888);
        drawOutputPixelBound(_srcCol-circleY, _srcRow-circleX, widthBot, widthTop, heightBot, heightTop, _outImage, _rgb888);
      }
    }

    void __attribute__((always_inline)) drawRgbTargetCenterLine(const int32_t _srcCol, 
                                                                const int32_t _srcRow,
                                                                const TrikCvImageBuffer& _outImage,
                                                                const uint32_t _rgb888)
    {
      const int32_t widthBot  = 0;
      const int32_t widthTop  = m_inImageDesc.m_width-1;
      const int32_t heightBot = 0;
      const int32_t heightTop = m_inImageDesc.m_height-1;

      for (int adj = 0; adj < 100; ++adj)
      {
        drawOutputPixelBound(_srcCol  , _srcRow-adj, widthBot, widthTop, heightBot, heightTop, _outImage, _rgb888);
        drawOutputPixelBound(_srcCol  , _srcRow+adj, widthBot, widthTop, heightBot, heightTop, _outImage, _rgb888);
      }
    }

    void __attribute__((always_inline)) fillImage(const TrikCvImageBuffer& _outImage, const uint32_t _rgb888)
    {
      const int32_t widthBot  = 0;
      const int32_t widthTop  = m_inImageDesc.m_width-1;
      const int32_t heightBot = 0;
      const int32_t heightTop = m_inImageDesc.m_height-1;

      for (int row = 0; row < 50; ++row)
      {
        for (int col = 0; col < 50; ++col)
        {
          drawOutputPixelBound(col, row, widthBot, widthTop, heightBot, heightTop, _outImage, _rgb888);
        }
      }

    }

    void __attribute__((always_inline)) drawRgbTargetHorizontalCenterLine(const int32_t _srcCol, 
                                                                const int32_t _srcRow,
                                                                const TrikCvImageBuffer& _outImage,
                                                                const uint32_t _rgb888)
    {
      const int32_t widthBot  = 0;
      const int32_t widthTop  = m_inImageDesc.m_width-1;
      const int32_t heightBot = 0;
      const int32_t heightTop = m_inImageDesc.m_height-1;

      for (int adj = 0; adj < 100; ++adj)
      {
        drawOutputPixelBound(_srcCol-adj  , _srcRow, widthBot, widthTop, heightBot, heightTop, _outImage, _rgb888);
        drawOutputPixelBound(_srcCol+adj  , _srcRow, widthBot, widthTop, heightBot, heightTop, _outImage, _rgb888);
      }
    }


    static bool __attribute__((always_inline)) detectHsvPixel(const uint32_t _hsv,
                                                              const uint64_t _hsv_range,
                                                              const uint32_t _hsv_expect)
    {
      const uint32_t u32_hsv_det = _cmpltu4(_hsv, _hill(_hsv_range))
                                 | _cmpgtu4(_hsv, _loll(_hsv_range));

      return (u32_hsv_det == _hsv_expect);
    }

    static uint64_t DEBUG_INLINE convert2xYuyvToRgb888(const uint32_t _yuyv)
    {
      const int64_t  s64_yuyv1   = _mpyu4ll(_yuyv,
                                             (static_cast<uint32_t>(static_cast<uint8_t>(409/4))<<24)
                                            |(static_cast<uint32_t>(static_cast<uint8_t>(298/4))<<16)
                                            |(static_cast<uint32_t>(static_cast<uint8_t>(516/4))<< 8)
                                            |(static_cast<uint32_t>(static_cast<uint8_t>(298/4))    ));
      const uint32_t u32_yuyv2   = _dotpus4(_yuyv,
                                             (static_cast<uint32_t>(static_cast<uint8_t>(-208/4))<<24)
                                            |(static_cast<uint32_t>(static_cast<uint8_t>(-100/4))<< 8));
      const uint32_t u32_rgb_h   = _add2(_packh2( 0,         _hill(s64_yuyv1)),
                                         (static_cast<uint32_t>(static_cast<uint16_t>(128/4 + (-128*409-16*298)/4))));
      const uint32_t u32_rgb_l   = _add2(_packlh2(u32_yuyv2, _loll(s64_yuyv1)),
                                          (static_cast<uint32_t>(static_cast<uint16_t>(128/4 + (+128*100+128*208-16*298)/4)<<16))
                                         |(static_cast<uint32_t>(static_cast<uint16_t>(128/4 + (-128*516-16*298)/4))));
      const uint32_t u32_y1y1    = _pack2(_loll(s64_yuyv1), _loll(s64_yuyv1));
      const uint32_t u32_y2y2    = _pack2(_hill(s64_yuyv1), _hill(s64_yuyv1));
      const uint32_t u32_rgb_p1h = _clr(_shr2(_add2(u32_rgb_h, u32_y1y1), 6), 16, 31);
      const uint32_t u32_rgb_p1l =      _shr2(_add2(u32_rgb_l, u32_y1y1), 6);
      const uint32_t u32_rgb_p2h = _clr(_shr2(_add2(u32_rgb_h, u32_y2y2), 6), 16, 31);
      const uint32_t u32_rgb_p2l =      _shr2(_add2(u32_rgb_l, u32_y2y2), 6);
      const uint32_t u32_rgb_p1 = _spacku4(u32_rgb_p1h, u32_rgb_p1l);
      const uint32_t u32_rgb_p2 = _spacku4(u32_rgb_p2h, u32_rgb_p2l);
      return _itoll(u32_rgb_p2, u32_rgb_p1);
    }

    static uint32_t DEBUG_INLINE convertRgb888ToHsv(const uint32_t _rgb888)
    {
      const uint32_t u32_rgb_or16    = _unpkhu4(_rgb888);
      const uint32_t u32_rgb_gb16    = _unpklu4(_rgb888);

      const uint32_t u32_rgb_max2    = _maxu4(_rgb888, _rgb888>>8);
      const uint32_t u32_rgb_max     = _clr(_maxu4(u32_rgb_max2, u32_rgb_max2>>8), 8, 31); // top 3 bytes were non-zeroes!
      const uint32_t u32_rgb_max_max = _pack2(u32_rgb_max, u32_rgb_max);

      const uint32_t u32_hsv_ooo_val_x256   = u32_rgb_max<<8; // get max in 8..15 bits

      const uint32_t u32_rgb_min2    = _minu4(_rgb888, _rgb888>>8);
      const uint32_t u32_rgb_min     = _minu4(u32_rgb_min2, u32_rgb_min2>>8); // top 3 bytes are zeroes
      const uint32_t u32_rgb_delta   = u32_rgb_max-u32_rgb_min;

      /* optimized by table based multiplication with power-2 divisor, simulate 255*(max-min)/max */
      const uint32_t u32_hsv_sat_x256       = s_mult255_div[u32_rgb_max]
                                            * u32_rgb_delta;

      /* optimized by table based multiplication with power-2 divisor, simulate 43*(med-min)/(max-min) */
      const uint32_t u32_hsv_hue_mult43_div = _pack2(s_mult43_div[u32_rgb_delta],
                                                     s_mult43_div[u32_rgb_delta]);
      int32_t s32_hsv_hue_x256;
      const uint32_t u32_rgb_cmp = _cmpeq2(u32_rgb_max_max, u32_rgb_gb16);
      if (u32_rgb_cmp == 0)
          s32_hsv_hue_x256 = static_cast<int32_t>((0x10000*0)/3)
                           + static_cast<int32_t>(_dotpn2(u32_hsv_hue_mult43_div,
                                                          _packhl2(u32_rgb_gb16, u32_rgb_gb16)));
      else if (u32_rgb_cmp == 1)
          s32_hsv_hue_x256 = static_cast<int32_t>((0x10000*2)/3)
                           + static_cast<int32_t>(_dotpn2(u32_hsv_hue_mult43_div,
                                                          _packlh2(u32_rgb_or16, u32_rgb_gb16)));
      else // 2, 3
          s32_hsv_hue_x256 = static_cast<int32_t>((0x10000*1)/3)
                           + static_cast<int32_t>(_dotpn2(u32_hsv_hue_mult43_div,
                                                          _pack2(  u32_rgb_gb16, u32_rgb_or16)));

      const uint32_t u32_hsv_hue_x256      = static_cast<uint32_t>(s32_hsv_hue_x256);
      const uint32_t u32_hsv_sat_hue_x256  = _pack2(u32_hsv_sat_x256, u32_hsv_hue_x256);

      const uint32_t u32_hsv               = _packh4(u32_hsv_ooo_val_x256, u32_hsv_sat_hue_x256);
      return u32_hsv;
    }

    void DEBUG_INLINE convertImageYuyvToHsv(const TrikCvImageBuffer& _inImage)
    {
      const uint32_t srcImageRowEffectiveSize       = m_inImageDesc.m_width*sizeof(uint16_t);
      const uint32_t srcImageRowEffectiveToFullSize = m_inImageDesc.m_lineLength - srcImageRowEffectiveSize;
      const int8_t* restrict srcImageRow      = _inImage.m_ptr;
      const int8_t* restrict srcImageTo       = srcImageRow + m_inImageDesc.m_lineLength*m_inImageDesc.m_height;
      uint64_t* restrict rgb888hsvptr         = s_rgb888hsv;

      assert(m_inImageDesc.m_height % 4 == 0); // verified in setup
#pragma MUST_ITERATE(4, ,4)
      while (srcImageRow != srcImageTo)
      {
        assert(reinterpret_cast<intptr_t>(srcImageRow) % 8 == 0); // let's pray...
        const uint64_t* restrict srcImageCol4 = reinterpret_cast<const uint64_t*>(srcImageRow);
        srcImageRow += srcImageRowEffectiveSize;

        assert(m_inImageDesc.m_width % 32 == 0); // verified in setup
#pragma MUST_ITERATE(32/4, ,32/4)
        while (reinterpret_cast<const int8_t*>(srcImageCol4) != srcImageRow)
        {
          const uint64_t yuyv2x = *srcImageCol4++;

          const uint64_t rgb12 = convert2xYuyvToRgb888(_loll(yuyv2x));
          *rgb888hsvptr++ = _itoll(_loll(rgb12), convertRgb888ToHsv(_loll(rgb12)));
          *rgb888hsvptr++ = _itoll(_hill(rgb12), convertRgb888ToHsv(_hill(rgb12)));

          const uint64_t rgb34 = convert2xYuyvToRgb888(_hill(yuyv2x));
          *rgb888hsvptr++ = _itoll(_loll(rgb34), convertRgb888ToHsv(_loll(rgb34)));
          *rgb888hsvptr++ = _itoll(_hill(rgb34), convertRgb888ToHsv(_hill(rgb34)));
        }

        srcImageRow += srcImageRowEffectiveToFullSize;
      }
    }

void clasterizePixel(const uint32_t _hsv)
{
}

void clasterizeImage()
{
      const uint64_t* restrict rgb888hsvptr = s_rgb888hsv;
      const uint32_t width          = m_inImageDesc.m_width;
      const uint32_t height         = m_inImageDesc.m_height;

      const uint64_t u64_hsv_range  = m_detectRange;
      const uint32_t u32_hsv_expect = m_detectExpected;

      assert(m_inImageDesc.m_height % 4 == 0); // verified in setup
#pragma MUST_ITERATE(4, ,4)
      for (uint32_t srcRow=0; srcRow < height; ++srcRow)
      {

        assert(m_inImageDesc.m_width % 32 == 0); // verified in setup
#pragma MUST_ITERATE(32, ,32)
        for (uint32_t srcCol=0; srcCol < width; ++srcCol)
        {
          const uint64_t rgb888hsv = *rgb888hsvptr++;
          clasterizePixel(rgb888hsv);
          const bool det = detectHsvPixel(_loll(rgb888hsv), u64_hsv_range, u32_hsv_expect);

        }
      }
}

    void DEBUG_INLINE proceedImageHsv(TrikCvImageBuffer& _outImage)
    {
      const uint64_t* restrict rgb888hsvptr = s_rgb888hsv;
      const uint32_t width          = m_inImageDesc.m_width;
      const uint32_t height         = m_inImageDesc.m_height;
      const uint32_t dstLineLength  = m_outImageDesc.m_lineLength;
      const uint32_t srcToDstShift  = m_srcToDstShift;
      const uint64_t u64_hsv_range  = m_detectRange;
      const uint32_t u32_hsv_expect = m_detectExpected;
      uint32_t targetPointsPerRow;
      uint32_t targetPointsCol;

      assert(m_inImageDesc.m_height % 4 == 0); // verified in setup
#pragma MUST_ITERATE(4, ,4)
      for (uint32_t srcRow=0; srcRow < height; ++srcRow)
      {
        const uint32_t dstRow = srcRow >> srcToDstShift;
        uint16_t* restrict dstImageRow = reinterpret_cast<uint16_t*>(_outImage.m_ptr + dstRow*dstLineLength);

        targetPointsPerRow = 0;
        targetPointsCol = 0;
        assert(m_inImageDesc.m_width % 32 == 0); // verified in setup
#pragma MUST_ITERATE(32, ,32)
        for (uint32_t srcCol=0; srcCol < width; ++srcCol)
        {
          const uint32_t dstCol    = srcCol >> srcToDstShift;
          const uint64_t rgb888hsv = *rgb888hsvptr++;

          const bool det = detectHsvPixel(_loll(rgb888hsv), u64_hsv_range, u32_hsv_expect);
          targetPointsPerRow += det;
          targetPointsCol += det?srcCol:0;
          writeOutputPixel(dstImageRow+dstCol, _hill(rgb888hsv));
        }
        m_targetX      += targetPointsCol;
        m_targetY      += srcRow*targetPointsPerRow;
        m_targetPoints += targetPointsPerRow;
      }
    }

    double mod(double a, double b)
    {
        while(a > b)
        {
            a -= b;
        }
        return a;
    }

    double sgn(double a)
    {
        return a >= 0 ? a: -a;
    }

  uint32_t get_img_color()
  {
    uint32_t rgb_result;

    const uint64_t* restrict img = s_rgb888hsv;
    int row_start_position = 90;
    int row_finish_position = 150;
    int column_start_position = 130;
    int column_finish_position = 190;

    U_Hsv8x3 pixel;
    for(int row = row_start_position; row < row_finish_position; row++)
      for(int column = column_start_position; column < column_finish_position; column++)
      {
        pixel.whole = _loll(img[row*img_width + column]);
        int hc = (pixel.parts.h >> 3);
        int sc = (pixel.parts.s >> 3);
        int vc = (pixel.parts.v >> 3);
        s_color[hc][sc][vc]++;
     }

      int max = 0;
      int max_hi;
      int max_si;
      int max_vi;

      for(int h = 0; h < 32; h++)
      {
        for(int s = 0; s < 32; s++)
        {
          for(int v = 0; v < 32; v++)
          {
              if(s_color[h][s][v] > max)
              {
                max = s_color[h][s][v];
                max_hi = h << 3;
                max_si = s << 3;
                max_vi = v << 3;
              }
          }
        }
      }

      double max_h = static_cast<double>(max_hi)*360.0f/256.0f;
      double max_s = static_cast<double>(max_si)/256.0f;
      double max_v = static_cast<double>(max_vi)/256.0f;
      double r;
      double g;
      double b;

      double c = max_v*max_s;
      double x = c*(1.0f-sgn(mod((max_h/60.0f),2.0f) - 1.0f));
      double m = max_v - c;

      if (max_h < 60)
      {
        r = c;
        g = x;
        b = 0;
      } else if(max_h < 120)
      {
        r = x;
        g = c;
        b = 0;
      } else if(max_h < 180)
      {
        r = 0;
        g = c;
        b = x;
      } else if(max_h < 240)
      {
        r = 0;
        g = x;
        b = c;
      } else if(max_h < 300)
      {
        r = x;
        g = 0;
        b = c;
      } else if(max_h < 360)
      {
        r = c;
        g = 0;
        b = x;
      }
      
    r = r + m;
    g = g + m;
    b = b + m;

    uint8_t ri = static_cast<uint8_t>(r*256);
    uint8_t gi = static_cast<uint8_t>(g*256);
    uint8_t bi = static_cast<uint8_t>(b*256);

    rgb_result = ((int32_t)ri << 16) + ((int32_t)gi << 8) + ((int32_t)bi);

    return rgb_result;
  }


  public:
    virtual bool setup(const TrikCvImageDesc& _inImageDesc,
                       const TrikCvImageDesc& _outImageDesc,
                       int8_t* _fastRam, size_t _fastRamSize)
    {
      m_inImageDesc  = _inImageDesc;
      m_outImageDesc = _outImageDesc;

      if (   m_inImageDesc.m_width < 0
          || m_inImageDesc.m_height < 0
          || m_inImageDesc.m_width  % 32 != 0
          || m_inImageDesc.m_height % 4  != 0)
        return false;

      for (m_srcToDstShift = 0; m_srcToDstShift < 32; ++m_srcToDstShift)
        if (   (m_inImageDesc.m_width >>m_srcToDstShift) <= m_outImageDesc.m_width
            && (m_inImageDesc.m_height>>m_srcToDstShift) <= m_outImageDesc.m_height)
          break;

      /* Static member initialization on first instance creation */
      if (s_mult43_div == NULL || s_mult255_div == NULL)
      {
        if (_fastRamSize < (1u<<8)*sizeof(*s_mult43_div) + (1u<<8)*sizeof(*s_mult255_div))
          return false;

        s_mult43_div  = reinterpret_cast<typeof(s_mult43_div)>(_fastRam);
        _fastRam += (1u<<8)*sizeof(*s_mult43_div);
        s_mult255_div = reinterpret_cast<typeof(s_mult255_div)>(_fastRam);
        _fastRam += (1u<<8)*sizeof(*s_mult255_div);

        s_mult43_div[0] = 0;
        s_mult255_div[0] = 0;
        for (uint32_t idx = 1; idx < (1u<<8); ++idx)
        {
          s_mult43_div[ idx] = (43u  * (1u<<8)) / idx;
          s_mult255_div[idx] = (255u * (1u<<8)) / idx;
        }
      }

      return true;
    }

    virtual bool run(const TrikCvImageBuffer& _inImage, TrikCvImageBuffer& _outImage,
                     const TrikCvAlgInArgs& _inArgs, TrikCvAlgOutArgs& _outArgs)
    {
      if (m_inImageDesc.m_height * m_inImageDesc.m_lineLength > _inImage.m_size)
        return false;
      if (m_outImageDesc.m_height * m_outImageDesc.m_lineLength > _outImage.m_size)
        return false;
      _outImage.m_size = m_outImageDesc.m_height * m_outImageDesc.m_lineLength;

      m_targetX = 0;
      m_targetY = 0;
      m_targetPoints = 0;

      memset(s_color, 0, sizeof(int)*32*32*32);
      uint32_t detectHueFrom = range<int32_t>(0, (static_cast<int32_t>(_inArgs.detectHueFrom) * 255) / 359, 255); // scaling 0..359 to 0..255
      uint32_t detectHueTo   = range<int32_t>(0, (static_cast<int32_t>(_inArgs.detectHueTo  ) * 255) / 359, 255); // scaling 0..359 to 0..255
      uint32_t detectSatFrom = range<int32_t>(0, (static_cast<int32_t>(_inArgs.detectSatFrom) * 255) / 100, 255); // scaling 0..100 to 0..255
      uint32_t detectSatTo   = range<int32_t>(0, (static_cast<int32_t>(_inArgs.detectSatTo  ) * 255) / 100, 255); // scaling 0..100 to 0..255
      uint32_t detectValFrom = range<int32_t>(0, (static_cast<int32_t>(_inArgs.detectValFrom) * 255) / 100, 255); // scaling 0..100 to 0..255
      uint32_t detectValTo   = range<int32_t>(0, (static_cast<int32_t>(_inArgs.detectValTo  ) * 255) / 100, 255); // scaling 0..100 to 0..255
      bool     autoDetectHsv = static_cast<bool>(_inArgs.autoDetectHsv); // true or false

      if (detectHueFrom <= detectHueTo)
      {
        m_detectRange = _itoll((detectValFrom<<16) | (detectSatFrom<<8) | detectHueFrom,
                               (detectValTo  <<16) | (detectSatTo  <<8) | detectHueTo  );
        m_detectExpected = 0x0;
      }
      else
      {
        assert(detectHueFrom > 0 && detectHueTo < 255);
        m_detectRange = _itoll((detectValFrom<<16) | (detectSatFrom<<8) | (detectHueTo  +1),
                               (detectValTo  <<16) | (detectSatTo  <<8) | (detectHueFrom-1));
        m_detectExpected = 0x1;
      }

#ifdef DEBUG_REPEAT
      for (unsigned repeat = 0; repeat < DEBUG_REPEAT; ++repeat) {
#endif


      if (m_inImageDesc.m_height > 0 && m_inImageDesc.m_width > 0)
      {
        convertImageYuyvToHsv(_inImage);

        proceedImageHsv(_outImage);

      }

#ifdef DEBUG_REPEAT
      } // repeat
#endif
      int32_t res_color = 0x000000;
      res_color = get_img_color();
      //draw taget pointer
      fillImage(_outImage, res_color);

      drawRgbTargetCenterLine(130, 120, _outImage, 0xff00ff);
      drawRgbTargetCenterLine(190, 120, _outImage, 0xff00ff);

/*
      drawRgbTargetCenterLine(90,  120, _outImage, 0xff00ff);
      drawRgbTargetCenterLine(230, 120, _outImage, 0xff00ff);
*/
      drawRgbTargetHorizontalCenterLine(160, 90, _outImage, 0xff00ff);
      drawRgbTargetHorizontalCenterLine(160, 150, _outImage, 0xff00ff);
/*
      drawRgbTargetHorizontalCenterLine(160, 50, _outImage, 0xff00ff);
      drawRgbTargetHorizontalCenterLine(160, 190, _outImage, 0xff00ff);
*/
/*
      if (m_targetPoints > 0)
      {
        const int32_t targetX = m_targetX/m_targetPoints;
        const int32_t targetY = m_targetY/m_targetPoints;

        assert(m_inImageDesc.m_height > 0 && m_inImageDesc.m_width > 0); // more or less safe since no target points would be detected otherwise
        const uint32_t targetRadius = std::ceil(std::sqrt(static_cast<float>(m_targetPoints) / 3.1415927f));

//        drawOutputCircle(targetX, targetY, targetRadius, _outImage, 0xffff00);

        _outArgs.targetX = ((targetX - static_cast<int32_t>(m_inImageDesc.m_width) /2) * 100*2) / static_cast<int32_t>(m_inImageDesc.m_width);
        _outArgs.targetY = ((targetY - static_cast<int32_t>(m_inImageDesc.m_height)/2) * 100*2) / static_cast<int32_t>(m_inImageDesc.m_height);
        _outArgs.targetSize = static_cast<uint32_t>(targetRadius*100*4) / static_cast<uint32_t>(m_inImageDesc.m_width + m_inImageDesc.m_height);
      }
      else
      {
        _outArgs.targetX = 0;
        _outArgs.targetY = 0;
        _outArgs.targetSize = 0;
      }
*/
      return true;
    }
};

uint16_t* restrict BallDetector<TRIK_VIDTRANSCODE_CV_VIDEO_FORMAT_YUV422,
                                TRIK_VIDTRANSCODE_CV_VIDEO_FORMAT_RGB565X>::s_mult43_div = NULL;
uint16_t* restrict BallDetector<TRIK_VIDTRANSCODE_CV_VIDEO_FORMAT_YUV422,
                                TRIK_VIDTRANSCODE_CV_VIDEO_FORMAT_RGB565X>::s_mult255_div = NULL;


} /* **** **** **** **** **** * namespace cv * **** **** **** **** **** */

} /* **** **** **** **** **** * namespace trik * **** **** **** **** **** */


#endif // !TRIK_VIDTRANSCODE_CV_INTERNAL_CV_BALL_DETECTOR_SEQPASS_HPP_

