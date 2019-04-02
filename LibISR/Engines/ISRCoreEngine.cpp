// Copyright 2014-2015 Isis Innovation Limited and the authors of LibISR

#include "ISRCoreEngine.h"
#include "../Utils/IOUtil.h"
#include "../Utils/NVTimer.h"

using namespace LibISR::Engine;
using namespace LibISR::Objects;


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
	printf("align colour image with depth image if need to\n");
	lowLevelEngine->prepareAlignedRGBDData(myhierarchy->levels[0].rgbd, myview->rawDepth, myview->rgb, &myview->calib->homo_depth_to_color);
	
	// build image hierarchy
	printf("build image hierarchy\n");
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

	printf("myview->alignedRgb->SetFrom\n");
	if (settings->useGPU){
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
	} 
	else myview->alignedRgb->SetFrom(myview->rgb, ORUtils::MemoryBlock<Vector4u>::CPU_TO_CPU);
	printf("After myview->alignedRgb->SetFrom\n");
	lowLevelEngine->computepfImageFromHistogram(myview->rgb, frame->histogram);

	ISRVisualisationState* myrendering = getRenderingState();
	ISRVisualisationState** tmprendering = new ISRVisualisationState*[settings->noTrackingObj];

	// raycast for rendering, not necessary if only track
	printf("raycast for rendering, not necessary if only track\n");
	for (int i = 0; i < settings->noTrackingObj; i++)
	{
		tmprendering[i] = new ISRVisualisationState(myview->rawDepth->noDims, settings->useGPU);
		tmprendering[i]->outputImage->Clear(0);
		visualizationEngine->updateMinmaxmImage(tmprendering[i]->minmaxImage, trackingState->getPose(i)->getH(), myview->calib->intrinsics_d.A, myview->depth->noDims);
		tmprendering[i]->minmaxImage->UpdateDeviceFromHost();
		visualizationEngine->renderObject(tmprendering[i], trackingState->getPose(i)->getInvH(), shapeUnion->getShape(0), myview->calib->intrinsics_d.getParam());
	}

	myrendering->outputImage->Clear(0);
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

