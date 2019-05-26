// Copyright 2014-2015 Isis Innovation Limited and the authors of LibISR

#pragma once
#include "../LibISRDefine.h"

#include "ISRHistogram.h"
#include "ISRCalib.h"

#include"../Utils/IOUtil.h"

namespace LibISR
{
	/**
	\brief
		view carries the basic image information
		including RGB, depth, raw depth and aligned RGB
		also includes the calibration parameters

		refactored: Jan/13/2015
	*/

	namespace Objects
	{

		class ISRView
		{
		public:
			enum InputDepthType
			{
				ISR_DISPARITY_DEPTH,
				ISR_SHORT_DEPTH,
			};

			InputDepthType inputDepthType;

			ISRCalib *calib;

			UChar4Image *rgb;
			ShortImage *rawDepth;

			FloatImage *depth;
			UChar4Image *alignedRgb;

			Float4Image *rgbd;

			ISRView(const ISRCalib &calib, Vector2i rgb_size, Vector2i d_size, bool  useGPU = false)
			{
				//remember to modify outImage after modfying this
				this->calib = new ISRCalib(calib);
				this->rgb = new UChar4Image(rgb_size, true,useGPU);
				this->rawDepth = new ShortImage(d_size, true, useGPU);
				this->depth = new FloatImage(d_size, true, useGPU);
				this->alignedRgb = new UChar4Image(d_size, true, useGPU);
				this->rgbd = new Float4Image(d_size, true, useGPU);
			}

			~ISRView()
			{
				delete calib;

				delete rgb;
				delete depth;

				delete rawDepth;
				delete alignedRgb;

				delete rgbd;
			}

			ISRView(const ISRView&);
			ISRView& operator=(const ISRView&);

			void RecordRaw(){
				char str[250];
				sprintf(str, "rgb.ppm");
				SaveImageToFile(rgb, str);
				sprintf(str, "raw-depth.ppm");
				SaveImageToFile(alignedRgb, str);
			}
		};
	}
}

