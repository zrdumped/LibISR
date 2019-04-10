// Copyright 2014-2015 Isis Innovation Limited and the authors of LibISR

#include "ISRVisualisationEngine_GPU.h"
#include "../shared/ISRVisualisationEngine_shared.h"

#include "../../Utils/IOUtil.h"

using namespace LibISR::Engine;
using namespace LibISR::Objects;

__global__ void renderObject_device(Vector4u* outImg, Vector2f* minmaxImg, Vector2i imgSize,const float* voxelData, Matrix4f invH, Vector4f invIntrinsic, Vector3f lightSource, float voxelrange);
__global__ void renderAsSDF_device(float* outSDFImg, Vector4f* outPtCloud, Vector4u* outImg, Vector2f* minmaxImg, Vector2i imgSize, const float* voxelData, Matrix4f invH, Vector4f invIntrinsic, Vector3f lightSource, float voxelrange);

//////////////////////////////////////////////////////////////////////////
// host functions
//////////////////////////////////////////////////////////////////////////


void LibISR::Engine::ISRVisualisationEngine_GPU::renderObject(Objects::ISRVisualisationState* rendering, const Matrix4f& invH, const Objects::ISRShape_ptr shape, const Vector4f& intrinsic)
{
	const float *voxelData = shape->getSDFVoxel();
	Vector4u *outimage = rendering->outputImage->GetData(MEMORYDEVICE_CUDA);
	Vector2f *minmaximg = rendering->minmaxImage->GetData(MEMORYDEVICE_CUDA);

	Vector2i imgSize = rendering->outputImage->noDims;
	
	dim3 blockSize(16, 16);
	dim3 gridSize((int)ceil((float)imgSize.x / (float)blockSize.x), (int)ceil((float)imgSize.y / (float)blockSize.y));

	Vector3f lightSource = -Vector3f(invH.getColumn(2));

	Vector4f invIntrinsic; 
	invIntrinsic.x = 1 / intrinsic.x; invIntrinsic.y = 1 / intrinsic.y;
	invIntrinsic.z = -intrinsic.z*invIntrinsic.x; invIntrinsic.w = -intrinsic.w*invIntrinsic.y;

	float one_on_top_of_maxVoxelRange = 1 / sqrtf(DT_VOL_SIZE*DT_VOL_SIZE + DT_VOL_SIZE*DT_VOL_SIZE + DT_VOL_SIZE*DT_VOL_SIZE);
	//printf("renderObject_device << <gridSize, blockSize >> >(outimage, minmaximg, imgSize, voxelData, invH, invIntrinsic, lightSource, one_on_top_of_maxVoxelRange);\n");
	renderObject_device << <gridSize, blockSize >> >(outimage, minmaximg, imgSize, voxelData, invH, invIntrinsic, lightSource, one_on_top_of_maxVoxelRange);
	//printf("ISRVisualisationEngine_GPU rendering->outputImage->UpdateHostFromDevice();\n");
	rendering->outputImage->UpdateHostFromDevice();
}

// TODO: change the code to proper GPU code, now it's actually cpp code!
void LibISR::Engine::ISRVisualisationEngine_GPU::renderDepth(UShortImage* renderedDepth, Objects::ISRVisualisationState* rendering, const Matrix4f& invH, const Objects::ISRShape_ptr shape, const Vector4f& intrinsic)
{
	const float *voxelData = shape->getSDFVoxel();
	ushort *outimage = renderedDepth->GetData(MEMORYDEVICE_CPU);
	Vector2f *minmaximg = rendering->minmaxImage->GetData(MEMORYDEVICE_CPU);

	Vector2i imgSize = rendering->outputImage->noDims;
	float mu = 0.5;
	Vector3f lightSource = -Vector3f(invH.getColumn(2));

	Vector4f invIntrinsic;
	invIntrinsic.x = 1 / intrinsic.x; invIntrinsic.y = 1 / intrinsic.y;
	invIntrinsic.z = -intrinsic.z*invIntrinsic.x; invIntrinsic.w = -intrinsic.w*invIntrinsic.y;

	float one_on_top_of_maxVoxelRange = 1 / sqrtf(DT_VOL_SIZE*DT_VOL_SIZE + DT_VOL_SIZE*DT_VOL_SIZE + DT_VOL_SIZE*DT_VOL_SIZE);


	for (int y = 0; y < imgSize.y; y++) for (int x = 0; x < imgSize.x; x++)
	{
		raycastAndRenderDepth(outimage, x, y, imgSize, voxelData, invH, invIntrinsic,minmaximg, one_on_top_of_maxVoxelRange);
	}
}

// TODO: change the code to proper GPU code, now it's actually cpp code!
void LibISR::Engine::ISRVisualisationEngine_GPU::renderDepthNormalAndObject(UShortImage* renderedDepth, UChar4Image* renderNormal, Objects::ISRVisualisationState* rendering, const Matrix4f& invH, const Objects::ISRShape_ptr shape, const Vector4f& intrinsic)
{
	const float *voxelData = shape->getSDFVoxel();
	ushort *outimageD = renderedDepth->GetData(MEMORYDEVICE_CPU);
	Vector4u* outimageGray = rendering->outputImage->GetData(MEMORYDEVICE_CPU);
	Vector4u* outimageNormal = renderNormal->GetData(MEMORYDEVICE_CPU);


	Vector2f *minmaximg = rendering->minmaxImage->GetData(MEMORYDEVICE_CPU);

	Vector2i imgSize = rendering->outputImage->noDims;
	Vector3f lightSource = -Vector3f(invH.getColumn(2));

	Vector4f invIntrinsic;
	invIntrinsic.x = 1 / intrinsic.x; invIntrinsic.y = 1 / intrinsic.y;
	invIntrinsic.z = -intrinsic.z*invIntrinsic.x; invIntrinsic.w = -intrinsic.w*invIntrinsic.y;

	float one_on_top_of_maxVoxelRange = 1 / sqrtf(DT_VOL_SIZE*DT_VOL_SIZE + DT_VOL_SIZE*DT_VOL_SIZE + DT_VOL_SIZE*DT_VOL_SIZE);


	for (int y = 0; y < imgSize.y; y++) for (int x = 0; x < imgSize.x; x++)
	{
		raycastAndRenderWithDepthAndSurfaceNormal(outimageD,outimageGray,outimageNormal, x, y, imgSize, voxelData, invH, invIntrinsic, minmaximg,lightSource, one_on_top_of_maxVoxelRange);
	}
}


void LibISR::Engine::ISRVisualisationEngine_GPU::renderAsSDF(FloatImage* SDFImage, Float4Image* ptCloud, Objects::ISRVisualisationState* rendering, const Matrix4f& invH, const Objects::ISRShape_ptr shape, const Vector4f& intrinsic)
{
	const float *voxelData = shape->getSDFVoxel();
	float *outSDF_ptr = SDFImage->GetData(MEMORYDEVICE_CUDA);
	Vector4f *outPtCloud_ptr = ptCloud->GetData(MEMORYDEVICE_CUDA);
	Vector4u* outimageGray = rendering->outputImage->GetData(MEMORYDEVICE_CUDA);
	Vector2f *minmaximg = rendering->minmaxImage->GetData(MEMORYDEVICE_CUDA);

	Vector2i imgSize = rendering->outputImage->noDims;
	float mu = 0.5;
	Vector3f lightSource = -Vector3f(invH.getColumn(2));

	Vector4f invIntrinsic;
	invIntrinsic.x = 1 / intrinsic.x; invIntrinsic.y = 1 / intrinsic.y;
	invIntrinsic.z = -intrinsic.z*invIntrinsic.x; invIntrinsic.w = -intrinsic.w*invIntrinsic.y;

	float one_on_top_of_maxVoxelRange = 1 / sqrtf(DT_VOL_SIZE*DT_VOL_SIZE + DT_VOL_SIZE*DT_VOL_SIZE + DT_VOL_SIZE*DT_VOL_SIZE);

	dim3 blockSize(16, 16);
	dim3 gridSize((int)ceil((float)imgSize.x / (float)blockSize.x), (int)ceil((float)imgSize.y / (float)blockSize.y));

	renderAsSDF_device << <gridSize, blockSize >> >(outSDF_ptr,outPtCloud_ptr,outimageGray, minmaximg, imgSize, voxelData, invH, invIntrinsic, lightSource, one_on_top_of_maxVoxelRange);
	rendering->outputImage->UpdateHostFromDevice();
}
//////////////////////////////////////////////////////////////////////////
// device functions
//////////////////////////////////////////////////////////////////////////

__global__ void renderObject_device(Vector4u* outImg, Vector2f* minmaxImg, Vector2i imgSize, const float* voxelData, Matrix4f invH, Vector4f invIntrinsic, Vector3f lightSource, float voxelrange)
{
	int x = threadIdx.x + blockIdx.x * blockDim.x, y = threadIdx.y + blockIdx.y * blockDim.y;
	if (x > imgSize.x - 1 || y > imgSize.y - 1) return;

	raycastAndRender(outImg, x, y, imgSize, voxelData, invH, invIntrinsic, minmaxImg, lightSource, voxelrange);
}

__global__ void renderAsSDF_device(float* outSDFImg, Vector4f* outPtCloud, Vector4u* outImg, Vector2f* minmaxImg, Vector2i imgSize, const float* voxelData, Matrix4f invH, Vector4f invIntrinsic, Vector3f lightSource, float voxelrange)
{
	int x = threadIdx.x + blockIdx.x * blockDim.x, y = threadIdx.y + blockIdx.y * blockDim.y;
	if (x > imgSize.x - 1 || y > imgSize.y - 1) return;

	raycaseAsSDF(outSDFImg, outPtCloud, outImg, x, y, imgSize, voxelData, invH, invIntrinsic, minmaxImg, lightSource, voxelrange);
}

