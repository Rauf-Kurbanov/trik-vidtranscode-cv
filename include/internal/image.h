#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "internal/cv_ball_detector_seqpass.hpp"
//#include "internal/stdcpp.hpp"
#include "trik_vidtranscode_cv.h"

#ifdef __cplusplus
extern "C" {
#endif

static const int cameraInputWidth = 320;
static const int cameraInputHeight = 240;
static const int cameraInputMaxgrey = 255;

typedef struct 
{
	int width;
	int height;
	int maxgrey;
	unsigned char* data;
	int flag;
}
MyImage;

typedef struct 
{
    uint32_t width;
    uint32_t height;
	int* data;
	int flag;
}
MyIntImage;

int readPgm(unsigned char* rawInputData, MyImage *image);
void createImage(int width, int height, MyImage *image);
void createSumImage(int width, int height, MyIntImage *image);
int freeImage(MyImage* image);
int freeSumImage(MyIntImage* image);
void setImage(int width, int height, MyImage *image);
void setSumImage(int width, int height, MyIntImage *image);

#ifdef __cplusplus
}
#endif

#endif
