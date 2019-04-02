// Copyright 2014-2015 Isis Innovation Limited and the authors of LibISR

#include "LibfreenectEngine.h"
#include "../Utils/IOUtil.h"

#include <iostream>
#include <thread>

#include <opencv2/cudawarping.hpp>
#include <opencv2/core/core.hpp>
#include <opencv/cv.hpp>
//#include <opencv2/core/cuda.hpp>

using namespace LibISRUtils;
using namespace LibISR::Objects;
using namespace cv;

class TestClass{
	public:
		int *signal;
		TestClass(){
			signal = new int(4);
			printf("TestClass Init\n");
			return;	
		}

		~TestClass(){
			delete signal;
			printf("TestClass Destroy\n");
			return;	
		}
};

class LibfreenectEngine::PrivateData {
public:
    libfreenect2::Freenect2Device *device;
    libfreenect2::PacketPipeline *pipeline;

    //TestClass* tc;

    libfreenect2::FrameMap frames;

    libfreenect2::Frame *colorFrame;
    //libfreenect2::Frame *ir;
    libfreenect2::Frame *depthFrame;
    libfreenect2::SyncMultiFrameListener *listener;

    libfreenect2::Registration *registration;
};


// void LibfreenectEngine::keepRecording(){
//     while(true){
//         printf("[keepRecording] record..\n");
//         data->listener->waitForNewFrame(data->frames);
//         data->colorFrame = data->frames[libfreenect2::Frame::Color];
//         cv::Mat rgbMat = cv::Mat((int)data->colorFrame->height, (int)data->colorFrame->width, CV_8UC4, data->colorFrame->data);
//         cv::imshow("rgb", rgbMat);
//         //cv::imwrite("rgb-test.jpg", rgbMat);
//         //printf("[Device Serial]%s\n", data->device->getSerialNumber());
//         //std::cout << "device serial: " << data->device->getSerialNumber() << std::endl;
//     }
// }

LibfreenectEngine::LibfreenectEngine(const char *calibFilename, const char *deviceURI, const bool useInternalCalibration,
	Vector2i requested_imageSize_rgb, Vector2i requested_imageSize_d)
	: ImageSourceEngine(calibFilename)
{
    //initialise();
    colorAvailable = true;
    depthAvailable = true;

    this->imageSize_rgb = requested_imageSize_rgb;
    this->imageSize_d = requested_imageSize_d;

    // data->listener->waitForNewFrame(data->frames);
    // data->colorFrame = data->frames[libfreenect2::Frame::Color];
    // cv::Mat rgbMat = cv::Mat((int)data->colorFrame->height, (int)data->colorFrame->width, CV_8UC4, data->colorFrame->data);
    // cv::imshow("rgb", rgbMat);
    // cv::imwrite("rgb-test.jpg", rgbMat);
    // printf("[Device Serial]%s\n", data->device->getSerialNumber());
    // std::cout << "device serial: " << data->device->getSerialNumber() << std::endl;
    std::thread keep = std::thread(&LibfreenectEngine::initialise, this);
    keep.detach();
    printf("LibfreenectEngine initialised\n");
}

void LibfreenectEngine::initialise(){
    printf("initialising Kinect2..\n");
    int deviceId = -1;


    data = new PrivateData();
    data->pipeline = new libfreenect2::CudaPacketPipeline(deviceId);
    //libfreenect2::PacketPipeline *pipeline = new libfreenect2::CpuPacketPipeline();

    //data->tc = new TestClass();

    libfreenect2::setGlobalLogger(libfreenect2::createConsoleLogger(libfreenect2::Logger::Debug));

    libfreenect2::Freenect2 freenect2;
    if(freenect2.enumerateDevices() == 0)
    {
        printf("[LibfreenectEngine]no device connected!\n");
        return;
    }

    data->device = freenect2.openDevice("007987464947", data->pipeline);
    if(data->device == 0)
    {
        printf("[LibfreenectEngine]failure opening device!\n");
        return;
    }
    
    //libfreenect2::Freenect2Device::Config config;
    // config.EnableBilateralFilter = true;
    // config.EnableEdgeAwareFilter = true;
    // config.MinDepth = 0.3f;
    // config.MaxDepth = 12.0f;
    //data->device->setConfiguration(config);

    int types = libfreenect2::Frame::Color | libfreenect2::Frame::Depth;
    data->listener = new libfreenect2::SyncMultiFrameListener(types);
    data->device->setColorFrameListener(data->listener);
    data->device->setIrAndDepthFrameListener(data->listener);

    data->registration = new libfreenect2::Registration(data->device->getIrCameraParams(), data->device->getColorCameraParams());
    

    // if (!data->device->start())    
    // {
    //     printf("[LibfreenectEngine]failure starting device!\n");
    //     return;
    // }
    if (!data->device->start()) 
    {
        printf("[LibfreenectEngine]failure starting device!\n");
        return;
    }

    //libfreenect2::Freenect2Device::IrCameraParams depthParams = data->device->getIrCameraParams();
    //libfreenect2::Freenect2Device::ColorCameraParams colorParams = data->device->getColorCameraParams();
    //data->registration = new libfreenect2::Registration(depthParams, colorParams);

    printf("Kinect2 initialised\n");
    while(inUse){
    }
}

LibfreenectEngine::~LibfreenectEngine(){
    printf("[LibfreenectEngine::~LibfreenectEngine()]\n");
    inUse = false;

    delete data->colorFrame;
    delete data->depthFrame;

    data->device->stop();
    data->device->close();

    delete data;
}

void LibfreenectEngine::getImages(LibISR::Objects::ISRView *out){
    //printf("[LibfreenectEngine::getImages]start\n");
    //printf("[Device Serial]%s\n", data->device->getSerialNumber());
    //data->listener->waitForNewFrame(data->frames);
    if (!data->listener->waitForNewFrame(data->frames, 10*1000)) // 10 sconds
    {
        printf("[LibfreenectEngine]Wait for frame times out\n");
        return;
    }
    //printf("[LibfreenectEngine::getImages]get new frame\n");
    data->colorFrame = data->frames[libfreenect2::Frame::Color];
    //ir = frames[libfreenect2::Frame::Ir];
    data->depthFrame = data->frames[libfreenect2::Frame::Depth];
    //libfreenect2::Frame undistorted(512, 424, 4), registered(512, 424, 4), depth2rgb(1920, 1080 + 2, 4);
    //data->registration->apply(data->colorFrame, data->depthFrame, &undistorted, &registered, true, &depth2rgb);

    
    //printf("[LibfreenectEngine::getImages]get rgb\n");
    Vector4u *rgb = out->rgb->GetData(MEMORYDEVICE_CPU);

    // Mat srcMat(1080, 1920, CV_8UC4, data->colorFrame->data);
    // cvtColor(srcMat, srcMat, COLOR_RGBA2BGRA);
	// Mat dstMat(424, 512, CV_8UC4);
	// cv::cuda::GpuMat mat1(srcMat);
	// cv::cuda::GpuMat mat2(dstMat);
	// mat1.upload(srcMat);
	// cv::cuda::resize(mat1, mat2, cvSize(512, 424));
	// mat1.download(srcMat);
	// mat2.download(dstMat);
    // memcpy(rgb, dstMat.data, out->rgb->dataSize * sizeof(Vector4u));

    for (int i = 0; i < out->rgb->noDims.x * out->rgb->noDims.y; i++)
	{
		Vector4u newPix; Vector4u oldPix = ((Vector4u*)data->colorFrame->data)[i];
		newPix.r = oldPix.b; newPix.g = oldPix.g; newPix.b = oldPix.r; newPix.a = oldPix.a;
		rgb[i] = newPix;
	}

    //printf("[LibfreenectEngine::getImages]get depth\n");
    short int *depth = out->rawDepth->GetData(MEMORYDEVICE_CPU);
    //printf("%d %d\n", sizeof(((float*)data->depthFrame->data)[0]), out->rawDepth->dataSize);
    for (int i = 0; i < out->rawDepth->noDims.x * out->rawDepth->noDims.y; i++)
	{
		float oldPix = ((float*)data->depthFrame->data)[i];
		depth[i] = (short int)(oldPix);
	}
    //memcpy(depth, data->depthFrame, out->rawDepth->dataSize * sizeof(float));

    out->inputDepthType = ISRView::ISR_SHORT_DEPTH;

    data->listener->release(data->frames);

    static int i = 0;
	if(i == 0){
        //Mat depth2rgbMat(1082, 1920, CV_32FC1, depth2rgb.data);
        //imwrite("d2rgb.jpg", depth2rgbMat / 4500.0f);

        out->RecordRaw();
        // imwrite("input_srgb.jpg", srcMat);
		// imwrite("input_drgb.jpg", dstMat);
        printf("[LibfreenectEngine::getImages]recording\n");
    }
    i++;
	return;
}

bool LibfreenectEngine::hasMoreImages(void) { return true; }
Vector2i LibfreenectEngine::getDepthImageSize(void) { return imageSize_d; }
Vector2i LibfreenectEngine::getRGBImageSize(void) { return imageSize_rgb; }

void LibfreenectEngine::test(){
    data->listener->waitForNewFrame(data->frames);
    data->colorFrame = data->frames[libfreenect2::Frame::Color];
    cv::Mat rgbMat = cv::Mat((int)data->colorFrame->height, (int)data->colorFrame->width, CV_8UC4, data->colorFrame->data);
    cv::imshow("rgb", rgbMat);
    //cv::imwrite("rgb-test.jpg", rgbMat);
    //printf("[Device Serial]%s\n", data->device->getSerialNumber());
    std::cout << "device serial: " << data->device->getSerialNumber() << std::endl;
}



