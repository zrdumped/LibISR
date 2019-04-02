// Copyright 2014-2015 Isis Innovation Limited and the authors of LibISR

#include "OpenNIEngine.h"

#include "../Utils/IOUtil.h"
#include <OpenNI.h>


using namespace LibISRUtils;
using namespace LibISR::Objects;

class OpenNIEngine::PrivateData {
public:
	PrivateData(void) : streams(NULL) {}
	openni::Device device;
	openni::VideoStream depthStream, colorStream;

	openni::VideoFrameRef depthFrame;
	openni::VideoFrameRef colorFrame;
	openni::VideoStream **streams;
};

static openni::VideoMode findBestMode(const openni::SensorInfo *sensorInfo, int requiredResolutionX = -1, int requiredResolutionY = -1, openni::PixelFormat requiredPixelFormat = (openni::PixelFormat) - 1)
{
	const openni::Array<openni::VideoMode> & modes = sensorInfo->getSupportedVideoModes();
	openni::VideoMode bestMode = modes[0];
	if(modes.getSize() == 1)
		return bestMode;
	for (int m = 0; m < modes.getSize(); ++m) {
		fprintf(stderr, "mode %i: %ix%i, %i %i\n", m, modes[m].getResolutionX(), modes[m].getResolutionY(), modes[m].getFps(), modes[m].getPixelFormat());
		const openni::VideoMode & curMode = modes[m];
		if ((requiredPixelFormat != -1) && (curMode.getPixelFormat() != requiredPixelFormat)) continue;

		bool acceptAsBest = false;
		if ((curMode.getResolutionX() == bestMode.getResolutionX()) &&
			(curMode.getFps() > bestMode.getFps())) {
			acceptAsBest = true;
		}
		else if ((requiredResolutionX <= 0) && (requiredResolutionY <= 0)) {
			if (curMode.getResolutionX() > bestMode.getResolutionX()) {
				acceptAsBest = true;
			}
		}
		else {
			int diffX_cur = abs(curMode.getResolutionX() - requiredResolutionX);
			int diffX_best = abs(bestMode.getResolutionX() - requiredResolutionX);
			int diffY_cur = abs(curMode.getResolutionY() - requiredResolutionY);
			int diffY_best = abs(bestMode.getResolutionY() - requiredResolutionY);
			if (requiredResolutionX > 0) {
				if (diffX_cur < diffX_best) {
					acceptAsBest = true;
				}
				if ((requiredResolutionY > 0) && (diffX_cur == diffX_best) && (diffY_cur < diffY_best)) {
					acceptAsBest = true;
				}
			}
			else if (requiredResolutionY > 0) {
				if (diffY_cur < diffY_best) {
					acceptAsBest = true;
				}
			}
		}
		if (acceptAsBest) bestMode = curMode;
	}
	//fprintf(stderr, "=> best mode: %ix%i, %i %i\n", bestMode.getResolutionX(), bestMode.getResolutionY(), bestMode.getFps(), bestMode.getPixelFormat());
	return bestMode;
}

OpenNIEngine::OpenNIEngine(const char *calibFilename, const char *deviceURI, const bool useInternalCalibration,
	Vector2i requested_imageSize_rgb, Vector2i requested_imageSize_d)
	: ImageSourceEngine(calibFilename)
{
	this->imageSize_d = Vector2i(0, 0);
	this->imageSize_rgb = Vector2i(0, 0);

	if (deviceURI == NULL) deviceURI = openni::ANY_DEVICE;

	data = new PrivateData();

	openni::Status rc = openni::STATUS_OK;

	rc = openni::OpenNI::initialize();
	printf("OpenNI: Initialization ... \n%s\n", openni::OpenNI::getExtendedError());

	rc = data->device.open(deviceURI);
	if (rc != openni::STATUS_OK)
	{
		printf("OpenNI: Device open failed:\n%s\n", openni::OpenNI::getExtendedError());
		openni::OpenNI::shutdown();
		return;
	}
	// if( data->device.isImageRegistrationModeSupported(openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR ) )    
    // {    
    //     data->device.setImageRegistrationMode( openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR );    
    // }

	openni::PlaybackControl *control = data->device.getPlaybackControl();
	if (control != NULL) {
		// this is a file! make sure we get every frame
		control->setSpeed(-1.0f);
		control->setRepeatEnabled(false);
	}

	rc = data->depthStream.create(data->device, openni::SENSOR_DEPTH);
	if (rc == openni::STATUS_OK)
	{
		openni::VideoMode depthMode = findBestMode(data->device.getSensorInfo(openni::SENSOR_DEPTH), requested_imageSize_d.x, requested_imageSize_d.y, openni::PIXEL_FORMAT_DEPTH_1_MM);
		imageSize_d.x = depthMode.getResolutionX();
		imageSize_d.y = depthMode.getResolutionY();
		rc = data->depthStream.setVideoMode(depthMode);
		if (rc != openni::STATUS_OK)
		{
			printf("OpenNI: Failed to set depth mode\n");
		}
		data->depthStream.setMirroringEnabled(false);

		rc = data->depthStream.start();
		if (rc != openni::STATUS_OK)
		{
			printf("OpenNI: Couldn't start depthStream stream:\n%s\n", openni::OpenNI::getExtendedError());
			data->depthStream.destroy();
		}

		if (useInternalCalibration) data->device.setImageRegistrationMode(openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR);

		printf("Initialised OpenNI depth camera with resolution: %d x %d\n", imageSize_d.x, imageSize_d.y);

		depthAvailable = true;
	}
	else
	{
		printf("OpenNI: Couldn't find depthStream stream:\n%s\n", openni::OpenNI::getExtendedError());
		depthAvailable = false;
	}

	rc = data->colorStream.create(data->device, openni::SENSOR_COLOR);
	if (rc == openni::STATUS_OK)
	{
		openni::VideoMode colourMode = findBestMode(data->device.getSensorInfo(openni::SENSOR_COLOR), requested_imageSize_rgb.x, requested_imageSize_rgb.y);
		this->imageSize_rgb.x = colourMode.getResolutionX();
		this->imageSize_rgb.y = colourMode.getResolutionY();
		rc = data->colorStream.setVideoMode(colourMode);
		if (rc != openni::STATUS_OK)
		{
			printf("OpenNI: Failed to set color mode\n");
		}
		data->colorStream.setMirroringEnabled(false);

		rc = data->colorStream.start();
		if (rc != openni::STATUS_OK)
		{
			printf("OpenNI: Couldn't start colorStream stream:\n%s\n", openni::OpenNI::getExtendedError());
			data->colorStream.destroy();
		}

		printf("Initialised OpenNI color camera with resolution: %d x %d\n", imageSize_rgb.x, imageSize_rgb.y);

		colorAvailable = true;
	}
	else
	{
		printf("OpenNI: Couldn't find colorStream stream:\n%s\n", openni::OpenNI::getExtendedError());
		colorAvailable = false;
	}

	if (!depthAvailable)
	{
		printf("OpenNI: No valid streams. Exiting\n");
		openni::OpenNI::shutdown();
		return;
	}

	data->streams = new openni::VideoStream*[2];
	if (depthAvailable) data->streams[0] = &data->depthStream;
	if (colorAvailable) data->streams[1] = &data->colorStream;
}

OpenNIEngine::~OpenNIEngine()
{
	if (depthAvailable)
	{
		data->depthStream.stop();
		data->depthStream.destroy();
	}
	if (colorAvailable)
	{
		data->colorStream.stop();
		data->colorStream.destroy();
	}
	data->device.close();

	delete[] data->streams;
	delete data;

	openni::OpenNI::shutdown();
}

void OpenNIEngine::getImages(LibISR::Objects::ISRView *out)
{
	int changedIndex, waitStreamCount;
	if (depthAvailable && colorAvailable) waitStreamCount = 2;
	else waitStreamCount = 1;

	openni::Status rc = openni::OpenNI::waitForAnyStream(data->streams, waitStreamCount, &changedIndex);
	if (rc != openni::STATUS_OK) { printf("OpenNI: Wait failed\n"); return /*false*/; }

	if (depthAvailable) data->depthStream.readFrame(&data->depthFrame);
	if (colorAvailable) data->colorStream.readFrame(&data->colorFrame);

	if (depthAvailable && !data->depthFrame.isValid()) return;
	if (colorAvailable && !data->colorFrame.isValid()) return;

	Vector4u *rgb = out->rgb->GetData(MEMORYDEVICE_CPU);
	if (colorAvailable)
	{
		//openni::VideoMode colorMode = data->colorStream.
		//fprintf(stderr, "mode: %ix%i, %i %i\n", colorMode.getResolutionX(), colorMode.getResolutionY(), colorMode.getFps(), colorMode.getPixelFormat());

		const openni::RGB888Pixel* colorImagePix = (const openni::RGB888Pixel*)data->colorFrame.getData();
		//RGB -> RGBA
		//printf("color frame size noDims x %d, y %d", out->rgb->noDims.x, out->rgb->noDims.y);
		
		for (int i = 0; i < out->rgb->noDims.x * out->rgb->noDims.y; i++)
		{
			Vector4u newPix; openni::RGB888Pixel oldPix = colorImagePix[i];
			newPix.r = oldPix.r; newPix.g = oldPix.g; newPix.b = oldPix.b; newPix.a = 255;
			rgb[i] = newPix;
		}
	}
	else memset(rgb, 0, out->rgb->dataSize * sizeof(Vector4u));

	short *depth = out->rawDepth->GetData(MEMORYDEVICE_CPU);
	if (depthAvailable)
	{
		const openni::DepthPixel* depthImagePix = (const openni::DepthPixel*)data->depthFrame.getData();
		memcpy(depth, depthImagePix, out->rawDepth->dataSize * sizeof(short));
	}
	else memset(depth, 0, out->rawDepth->dataSize * sizeof(short));

	out->inputDepthType = ISRView::ISR_SHORT_DEPTH;
	static int i = 0;
	if(i == 20){
		cv::Mat rgbMat = cv::Mat(data->colorFrame.getHeight(), data->colorFrame.getWidth(), CV_8UC3, (openni::RGB888Pixel*)data->colorFrame.getData());
		cv::imshow("rgb", rgbMat);
		cv::imwrite("cv-rgb.jpg", rgbMat);

		out->RecordRaw();
	}
	i++;
	return /*true*/;
}

bool OpenNIEngine::hasMoreImages(void) { return true; }
Vector2i OpenNIEngine::getDepthImageSize(void) { return imageSize_d; }
Vector2i OpenNIEngine::getRGBImageSize(void) { return imageSize_rgb; }



