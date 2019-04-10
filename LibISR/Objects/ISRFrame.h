// Copyright 2014-2015 Isis Innovation Limited and the authors of LibISR

#pragma once
#include "../LibISRDefine.h"
#include "ISRImageHierarchy.h"
#include "ISRHistogram.h"
#include "ISRView.h"
#include "ISRVisualisationState.h"

namespace LibISR
{
	namespace Objects
	{
		/** \brief
		Represents intermediate data for a RGB-D frame,
		including occlusion map, pointCloud

		refactored: Jan/13/2015
		*/
		class ISRFrame
		{
		public:

			int height;
			int width;

			Vector2i depth_size;
			Vector2i rgb_size;

			ISRView* view;

			ISRHistogram *histogram;
			Float4Image *ptCloud;

			ISRImageHierarchy *imgHierarchy;
			ISRImageHierarchy::ImageLevel *currentLevel;

			ISRVisualisationState *rendering;

			ISRFrame(const ISRCalib &calib, Vector2i color_size, Vector2i d_size, bool useGPU = false, int noHierarchy = 3)
			{
				depth_size = d_size;
				rgb_size = color_size;

				ptCloud = new Float4Image(d_size,true, useGPU);
				printf("[ISRFrame] color_size %dx%d, depth_size %dx%d\n",
					color_size.x, color_size.y, d_size.x, d_size.y);
				view = new ISRView(calib, color_size, d_size, useGPU);
				imgHierarchy = new ISRImageHierarchy(d_size, noHierarchy, useGPU);
				rendering = new ISRVisualisationState(color_size, useGPU);
				//Resize(view->rgb->GetData(MEMORYDEVICE_CUDA), view->rgb->GetData(MEMORYDEVICE_CUDA));
			}

			~ISRFrame()
			{

				delete ptCloud;
				delete view;
				delete imgHierarchy;
				delete rendering;
			}
		};
	}
}

