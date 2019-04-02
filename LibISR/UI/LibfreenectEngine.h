// Copyright 2014-2015 Isis Innovation Limited and the authors of LibISR

#pragma once

#include "ImageSourceEngine.h"

#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include <libfreenect2/registration.h>
#include <libfreenect2/packet_pipeline.h>
#include <libfreenect2/logger.h>

#include <thread>

namespace LibISRUtils
{
	class LibfreenectEngine : public ImageSourceEngine
	{
	private:
		//static LibfreenectEngine *instance;

		class PrivateData;
		PrivateData *data;
		Vector2i imageSize_rgb, imageSize_d;
		bool colorAvailable, depthAvailable;

		bool inUse = true;

		void initialise();
		
	public:
		// static LibfreenectEngine* Instance(const char *calibFilename, const char *deviceURI = NULL, const bool useInternalCalibration = false) {
		// 	LibfreenectEngine *instance = NULL;
		// 	if (instance == NULL) instance = new LibfreenectEngine(calibFilename, deviceURI, useInternalCalibration);
		// 	return instance;
		// }
		LibfreenectEngine(const char *calibFilename, const char *deviceURI = NULL, const bool useInternalCalibration = false,
			Vector2i imageSize_rgb = Vector2i(1920, 1080), Vector2i imageSize_d = Vector2i(512, 424));

		~LibfreenectEngine();

        void test();

		bool hasMoreImages(void);
		void getImages(LibISR::Objects::ISRView *out);
		Vector2i getDepthImageSize(void);
		Vector2i getRGBImageSize(void);
		PrivateData* getPrivateData(void){return data;}
	};
}

