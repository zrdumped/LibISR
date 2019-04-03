// Copyright 2014-2015 Isis Innovation Limited and the authors of LibISR

#include "ImageSourceEngine.h"

#include "../Utils/IOUtil.h"

#include <stdio.h>

using namespace LibISRUtils;
using namespace LibISR::Objects;

ImageSourceEngine::ImageSourceEngine(const char *calibFilename)
{
	 readRGBDCalib(calibFilename, calib);
	 //update homography manualy
	 calib.homo_depth_to_color.H = calib.intrinsics_rgb.A * calib.trafo_rgb_to_depth.R * calib.intrinsics_d.invA;
	 calib.homo_depth_to_color.T = calib.intrinsics_rgb.A * calib.trafo_rgb_to_depth.T / 1000.0f;
	
	// printf("H: %f %f %f\n", calib.intrinsics_rgb.A.m00, calib.intrinsics_rgb.A.m01, calib.intrinsics_rgb.A.m02);
	// printf("H: %f %f %f\n", calib.intrinsics_rgb.A.m00, calib.intrinsics_rgb.A.m01, calib.intrinsics_rgb.A.m02);
	// printf("H: %f %f %f\n", calib.intrinsics_rgb.A.m00, calib.intrinsics_rgb.A.m01, calib.intrinsics_rgb.A.m02);

	// printf("H: %f %f %f\n", calib.homo_depth_to_color.H.m00, calib.homo_depth_to_color.H.m10, calib.homo_depth_to_color.H.m20);
	// printf("H: %f %f %f\n", calib.homo_depth_to_color.H.m01, calib.homo_depth_to_color.H.m11, calib.homo_depth_to_color.H.m21);
	// printf("H: %f %f %f\n", calib.homo_depth_to_color.H.m02, calib.homo_depth_to_color.H.m12, calib.homo_depth_to_color.H.m22);
	// printf("T: %f %f %f\n", calib.homo_depth_to_color.T.x, calib.homo_depth_to_color.T.y, calib.homo_depth_to_color.T.z);
}


ImageFileReader::ImageFileReader(const char *calibFilename, const char *rgbImageMask, const char *depthImageMask)
	: ImageSourceEngine(calibFilename)
{
	strncpy(this->rgbImageMask, rgbImageMask, BUF_SIZE);
	strncpy(this->depthImageMask, depthImageMask, BUF_SIZE);

	currentFrameNo = 0;
	cachedFrameNo = -1;

	cached_rgb = NULL;
	cached_depth = NULL;
}

ImageFileReader::~ImageFileReader()
{
	delete cached_rgb;
	delete cached_depth;
}

void ImageFileReader::loadIntoCache(void)
{
	if ((cached_rgb != NULL) || (cached_depth != NULL)) return;

	cached_rgb = new UChar4Image(true,false);
	cached_depth = new ShortImage(true,false);

	char str[2048];
	sprintf(str, rgbImageMask, currentFrameNo);
	if (!ReadImageFromFile(cached_rgb, str)) {
		delete cached_rgb; cached_rgb = NULL;
		printf("error reading file '%s'\n", str);
	}
	sprintf(str, depthImageMask, currentFrameNo);
	if (!ReadImageFromFile(cached_depth, str)) {
		delete cached_depth; cached_depth = NULL;
		printf("error reading file '%s'\n", str);
	}
}

bool ImageFileReader::hasMoreImages(void)
{
	loadIntoCache();
	return ((cached_rgb != NULL) && (cached_depth != NULL));
}

void ImageFileReader::getImages(ISRView *out)
{
	bool bUsedCache = false;
	if (cached_rgb != NULL) {
		out->rgb->SetFrom(cached_rgb, ORUtils::MemoryBlock<Vector4u>::CPU_TO_CPU);
		delete cached_rgb;
		cached_rgb = NULL;
		bUsedCache = true;
	}
	if (cached_depth != NULL) {
		out->rawDepth->SetFrom(cached_depth, ORUtils::MemoryBlock<short>::CPU_TO_CPU);
		delete cached_depth;
		cached_depth = NULL;
		bUsedCache = true;
	}

	if (!bUsedCache) {
		char str[2048];
		sprintf(str, rgbImageMask, currentFrameNo);
		if (!ReadImageFromFile(out->rgb, str)) {
			printf("error reading file '%s'\n", str);
		}

		sprintf(str, depthImageMask, currentFrameNo);
		if (!ReadImageFromFile(out->rawDepth, str)) {
			printf("error reading file '%s'\n", str);
		}
	}

	if (calib.disparityCalib.params.y == 0) out->inputDepthType = ISRView::ISR_SHORT_DEPTH;
	else out->inputDepthType = ISRView::ISR_DISPARITY_DEPTH;

	++currentFrameNo;
}

Vector2i ImageFileReader::getDepthImageSize(void)
{
	loadIntoCache();
	return cached_depth->noDims;
}

Vector2i ImageFileReader::getRGBImageSize(void)
{
	loadIntoCache();
	return cached_rgb->noDims;
}
