// Copyright 2014-2015 Isis Innovation Limited and the authors of LibISR

#include "ISRCoreEngine.h"
#include "../Utils/IOUtil.h"
#include "../Utils/NVTimer.h"

#include <opencv2/core/core.hpp>
#include <opencv/cv.hpp>

using namespace LibISR::Engine;
using namespace LibISR::Objects;

using namespace cv;


LibISR::Engine::ISRCoreEngine::ISRCoreEngine(const ISRLibSettings *settings, const ISRCalib *calib, Vector2i d_size, Vector2i rgb_size)
{
	//printf("LibISR::Engine::ISRCoreEngine::ISRCoreEngine\N");
	this->settings = new ISRLibSettings(*settings);
	this->shapeUnion = new ISRShapeUnion(settings->noTrackingObj, settings->useGPU);
	this->trackingState = new ISRTrackingState(settings->noTrackingObj,settings->useGPU);
	this->frame = new ISRFrame(*calib, rgb_size, d_size, settings->useGPU);
	this->frame->histogram = new ISRHistogram(settings->noHistogramDim, settings->useGPU);

	if (settings->useGPU)
	{
		this->lowLevelEngine = new ISRLowlevelEngine_GPU();
		this->visualizationEngine = new ISRVisualisationEngine_GPU();
		this->tracker = new ISRRGBDTracker_GPU(settings->noTrackingObj,d_size);
	}
	else
	{
		this->lowLevelEngine = new ISRLowlevelEngine_CPU();
		this->visualizationEngine = new ISRVisualisationEngine_CPU();
		this->tracker = new ISRRGBDTracker_CPU(settings->noTrackingObj);
	}
	
	maxposediff = 0;
	needStarTracker = false;
}

static inline float interpolate(float val, float y0, float x0, float y1, float x1) {
	return (val - x0)*(y1 - y0) / (x1 - x0) + y0;
}

static inline float base(float val) {
	if (val <= -0.75f) return 0.0f;
	else if (val <= -0.25f) return interpolate(val, 0.0f, -0.75f, 1.0f, -0.25f);
	else if (val <= 0.25f) return 1.0f;
	else if (val <= 0.75f) return interpolate(val, 1.0f, 0.25f, 0.0f, 0.75f);
	else return 0.0;
}

void LibISR::Engine::ISRCoreEngine::processFrame(void)
{
	StopWatchInterface *timer;
	sdkCreateTimer(&timer);

	ISRView* myview = getView();
	ISRImageHierarchy* myhierarchy = getImageHierarchy();
	
	myhierarchy->levels[0].boundingbox = lowLevelEngine->findBoundingBoxFromCurrentState(trackingState, myview->calib->intrinsics_d.A, myview->depth->noDims);
	myhierarchy->levels[0].intrinsic = myview->calib->intrinsics_d.getParam();

	if (settings->useGPU)
	{
		myview->rawDepth->UpdateDeviceFromHost();
		myview->rgb->UpdateDeviceFromHost();
	}

	// align colour image with depth image if need to
	//printf("align colour image with depth image if need to\n");
	//printf("data%d\n", myview->rawDepth->GetData());
	lowLevelEngine->prepareAlignedRGBDData(myhierarchy->levels[0].rgbd, myview->rawDepth, myview->rgb, &myview->calib->homo_depth_to_color);

	static int i = 0;
	if(i==0){
	char str[250];
	float lims[2], scale;
	int dataSize = myhierarchy->levels[0].rgbd->dataSize;
	myhierarchy->levels[0].rgbd->UpdateHostFromDevice();
	Vector4f *destUC4 = myhierarchy->levels[0].rgbd->GetData(MEMORYDEVICE_CPU);
	short * rawdepth = myview->rawDepth->GetData(MEMORYDEVICE_CPU);
	lims[0] = 100000.0f; lims[1] = -100000.0f;

	for (int idx = 0; idx < dataSize; idx++)
	{
		float sourceVal = destUC4[idx].a;
		//short int depth = rawdepth[idx];
		//printf("%d ", depth);
		//if (sourceVal>65530) sourceVal = 0.0f;
		if (sourceVal > 0.0f) { lims[0] = MIN(lims[0], sourceVal); lims[1] = MAX(lims[1], sourceVal); }
	}

	printf("scale: %f - %f \n", lims[0], lims[1]);

	scale = ((lims[1] - lims[0]) != 0) ? 1.0f / (lims[1] - lims[0]) : 1.0f / lims[1];

	if (lims[0] == lims[1]) return;
	cv::Mat dstMat(424, 512, CV_8UC4, Scalar(0, 0, 0, 0));

	//sprintf(str, "depth-in-core.ppm");
	//SaveImageToFile(myview->rawDepth, str);
	//sprintf(str, "rgbd.ppm");
	//SaveImageToFile(myhierarchy->levels[0].rgbd, str);

	for (int idx = 0; idx < dataSize; idx++)
	{
		float sourceVal = destUC4[idx].a;
		if (sourceVal > 0.0f)
		{
			sourceVal = (sourceVal - lims[0]) * scale;
			dstMat.at<Vec4b>(idx / 512, idx % 512)[0] = (uchar)(base(sourceVal - 0.5f) * 127.5f + destUC4[idx].r*0.5f);
			dstMat.at<Vec4b>(idx / 512, idx % 512)[1] = (uchar)(base(sourceVal) * 127.5f + destUC4[idx].g*0.5f);
			dstMat.at<Vec4b>(idx / 512, idx % 512)[2] = (uchar)(base(sourceVal + 0.5f) * 127.5f + destUC4[idx].b*0.5f);
			dstMat.at<Vec4b>(idx / 512, idx % 512)[3] = 255;
		}else{
			dstMat.at<Vec4b>(idx / 512, idx % 512)[0] = 0;//(uchar)(base(sourceVal - 0.5f) * 127.5f + destUC4[idx].r*0.5f);
			dstMat.at<Vec4b>(idx / 512, idx % 512)[1] = 0;// (uchar)(base(sourceVal) * 127.5f + destUC4[idx].g*0.5f);
			dstMat.at<Vec4b>(idx / 512, idx % 512)[2] = 0;//(uchar)(base(sourceVal + 0.5f) * 127.5f + destUC4[idx].b*0.5f);
			dstMat.at<Vec4b>(idx / 512, idx % 512)[3] = 255;
		}
	}
	cv::imwrite("rgbd.jpg", dstMat);
	}i++;



	// build image hierarchy
	//printf("build image hierarchy\n");
	for (int i = 1; i < myhierarchy->noLevels; i++)
	{
		myhierarchy->levels[i].intrinsic = myhierarchy->levels[i - 1].intrinsic / 2;
		myhierarchy->levels[i].boundingbox = myhierarchy->levels[i - 1].boundingbox / 2;
		lowLevelEngine->subsampleImageRGBDImage(myhierarchy->levels[i].rgbd, myhierarchy->levels[i - 1].rgbd);
	}
	trackingState->boundingBox = myhierarchy->levels[0].boundingbox;

    // set hierarchy to work on
	int lvlnum; lvlnum = settings->useGPU ? 1 : myhierarchy->noLevels - 1;
	ISRImageHierarchy::ImageLevel& lastLevel = myhierarchy->levels[lvlnum];
	frame->currentLevel = &lastLevel;

    // prepare point cloud
	lowLevelEngine->preparePointCloudFromAlignedRGBDImage(frame->ptCloud, lastLevel.rgbd, frame->histogram, lastLevel.intrinsic, lastLevel.boundingbox);

	if (needStarTracker)
	{
		tracker->TrackObjects(frame, shapeUnion, trackingState);
	}

	//printf("myview->alignedRgb->SetFrom\n");
	//lowLevelEngine->
	//if (settings->useGPU){
		myview->alignedRgb->SetFrom(myview->rgb, ORUtils::MemoryBlock<Vector4u>::CUDA_TO_CPU);
		// if(myview->rgb->dataSize <= myview->alignedRgb->dataSize)
		// 	myview->alignedRgb->SetFrom(myview->rgb, ORUtils::MemoryBlock<Vector4u>::CUDA_TO_CPU);
		// else{
		// 	LibISR::Engine::MatrixAssist* ma = new LibISR::Engine::MatrixAssist();
		// 	UChar4Image * resizedRgb = new UChar4Image(Vector2i(640, 480), true, true);
		// 	ma->Resize(myview->rgb->GetData(MEMORYDEVICE_CUDA), resizedRgb->GetData(MEMORYDEVICE_CUDA));
		// 	myview->alignedRgb->SetFrom(resizedRgb, ORUtils::MemoryBlock<Vector4u>::CUDA_TO_CPU);
		// 	static int i = 0;
		// 	if(i == 20){
		// 		char str[250];
		// 		sprintf(str, "r_before.ppm");
		// 		SaveImageToFile(myview->rgb, str);
		// 		sprintf(str, "r_after.ppm");
		// 		SaveImageToFile(resizedRgb, str);
		// 		printf("saved========\n");
		// 	}
		// 	i++;
		// }
	//} 
	//else myview->alignedRgb->SetFrom(myview->rgb, ORUtils::MemoryBlock<Vector4u>::CPU_TO_CPU);
	//printf("After myview->alignedRgb->SetFrom\n");
	lowLevelEngine->computepfImageFromHistogram(myview->rgb, frame->histogram);

	ISRVisualisationState* myrendering = getRenderingState();
	ISRVisualisationState** tmprendering = new ISRVisualisationState*[settings->noTrackingObj];

	// raycast for rendering, not necessary if only track
	//printf("raycast for rendering, not necessary if only track\n");
	for (int i = 0; i < settings->noTrackingObj; i++)
	{
		tmprendering[i] = new ISRVisualisationState(myview->rgb->noDims, settings->useGPU);
		tmprendering[i]->outputImage->Clear(0);
		//[MODIFIED]depth->rgb
		visualizationEngine->updateMinmaxmImage(tmprendering[i]->minmaxImage, trackingState->getPose(i)->getH(), myview->calib->intrinsics_d.A, myview->rgb->noDims);
		tmprendering[i]->minmaxImage->UpdateDeviceFromHost();
		visualizationEngine->renderObject(tmprendering[i], trackingState->getPose(i)->getInvH(), shapeUnion->getShape(0), myview->calib->intrinsics_d.getParam());
	}

	myrendering->outputImage->Clear(0);

	// UChar4Image** tmprendering_out_1080 = new UChar4Image*[settings->noTrackingObj];
	// for(int j = 0; j < settings.noTrackingObj; j++){
	// 	tmprendering_out_1080[j] = new UChar4Image(Vector2i(1920, 1080) , true,useGPU);
	// 	Vector4u * 1080_image = tmprendering_out_1080[j]->GetData(MEMORYDEVICE_CPU);
	// 	Vector4u * 424_image = tmprendering[j]->outputImage->GetData(MEMORYDEVICE_CPU);

	// }

	for (int i = 0; i < myrendering->outputImage->dataSize; i++)
	{
		for (int j = 0; j < settings->noTrackingObj; j++)
		{
			myrendering->outputImage->GetData(MEMORYDEVICE_CPU)[i] += tmprendering[j]->outputImage->GetData(MEMORYDEVICE_CPU)[i] / settings->noTrackingObj;
		}

		if (myrendering->outputImage->GetData(MEMORYDEVICE_CPU)[i] != Vector4u(0, 0, 0, 0))
		{
			myview->alignedRgb->GetData(MEMORYDEVICE_CPU)[i] = myrendering->outputImage->GetData(MEMORYDEVICE_CPU)[i] / 5 * 4 + myview->alignedRgb->GetData(MEMORYDEVICE_CPU)[i] / 5;
			//myview->alignedRgb->GetData(MEMORYDEVICE_CPU)[i] = myrendering->outputImage->GetData(MEMORYDEVICE_CPU)[i];

		}
	}

	for (int i = 0; i < settings->noTrackingObj; i++) delete tmprendering[i];
	delete[] tmprendering;
}

