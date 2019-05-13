// Copyright 2014-2015 Isis Innovation Limited and the authors of LibISR

#pragma once

#include "../ISRLowlevelEngine.h"

namespace LibISR
{
	namespace Engine
	{
		class ISRLowlevelEngine_GPU:public ISRLowlevelEngine
		{
		public:
			void computepfImageFromHistogram(UChar4Image *rgb_in, Objects::ISRHistogram *histogram);

			void prepareAlignedRGBDData(Float4Image *outimg, UChar4Image * alignedRGB, ShortImage *raw_depth_in, UChar4Image *rgb_in, Objects::ISRExHomography *home);

			void subsampleImageRGBDImage(Float4Image *outimg, Float4Image *inimg);

			void preparePointCloudFromAlignedRGBDImage(Float4Image *ptcloud_out, Float4Image *inimg, Objects::ISRHistogram *histogram, const Vector4f &intrinsic, const Vector4i &boundingbox);

			ISRLowlevelEngine_GPU(){}
			~ISRLowlevelEngine_GPU(){}
		};

	}
}
