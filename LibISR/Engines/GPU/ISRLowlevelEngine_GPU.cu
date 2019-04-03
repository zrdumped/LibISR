// Copyright 2014-2015 Isis Innovation Limited and the authors of LibISR

#include "ISRLowlevelEngine_GPU.h"
#include "../shared/ISRLowlevelEngine_shared.h"

#include "../../Utils/IOUtil.h"

using namespace LibISR;
using namespace LibISR::Engine;
using namespace LibISR::Objects;

__global__ void subsampleImageRGBDImage_device(const Vector4f *imageData_in, Vector2i oldDims, Vector4f *imageData_out, Vector2i newDims);

__global__ void prepareAlignedRGBDData_device(Vector4f* rgbd_out, const short *depth_in, const Vector4u *rgb_in, const Vector2i imgSize, Matrix3f H, Vector3f T);

__global__ void preparePointCloudFromAlignedRGBDImage_device(Vector4f* ptcloud_out, Vector4f* inimg, float* histogram, Vector4f intrinsic, Vector4i boundingbox, Vector2i imgSize, int histBins);

__global__ void computepfImageFromHistogram_device(Vector4u* inimg, float* histogram, Vector2i imgSize, int histBins);

//////////////////////////////////////////////////////////////////////////
// host functions
//////////////////////////////////////////////////////////////////////////

void LibISR::Engine::ISRLowlevelEngine_GPU::subsampleImageRGBDImage(Float4Image *outimg, Float4Image *inimg)
{
	Vector2i oldDims = inimg->noDims;
	Vector2i newDims; newDims.x = inimg->noDims.x / 2; newDims.y = inimg->noDims.y / 2;
	outimg->ChangeDims(newDims);

	const Vector4f *imageData_in = inimg->GetData(MEMORYDEVICE_CUDA);
	Vector4f *imageData_out = outimg->GetData(MEMORYDEVICE_CUDA);

	dim3 blockSize(16, 16);
	dim3 gridSize((int)ceil((float)newDims.x / (float)blockSize.x), (int)ceil((float)newDims.y / (float)blockSize.y));

	subsampleImageRGBDImage_device << <gridSize, blockSize >> >(imageData_in, oldDims, imageData_out, newDims);
}

void LibISR::Engine::ISRLowlevelEngine_GPU::prepareAlignedRGBDData(Float4Image *outimg, ShortImage *raw_depth_in, UChar4Image *rgb_in, Objects::ISRExHomography *home)
{
	int w = raw_depth_in->noDims.width;
	int h = raw_depth_in->noDims.height;

	short* depth_in_ptr = raw_depth_in->GetData(MEMORYDEVICE_CUDA);
	Vector4u* rgb_in_ptr = rgb_in->GetData(MEMORYDEVICE_CUDA);
	Vector4f* rgbd_out_ptr = outimg->GetData(MEMORYDEVICE_CUDA);

	dim3 blockSize(16, 16);
	dim3 gridSize((int)ceil((float)w / (float)blockSize.x), (int)ceil((float)h / (float)blockSize.y));

	prepareAlignedRGBDData_device << <gridSize, blockSize >> >(rgbd_out_ptr, depth_in_ptr, rgb_in_ptr, rgb_in->noDims, home->H, home->T);
}

void LibISR::Engine::ISRLowlevelEngine_GPU::preparePointCloudFromAlignedRGBDImage(Float4Image *ptcloud_out, Float4Image *inimg, Objects::ISRHistogram *histogram, const Vector4f &intrinsic, const Vector4i &boundingbox)
{
	if (inimg->noDims != ptcloud_out->noDims) ptcloud_out->ChangeDims(inimg->noDims);
	
	int w = inimg->noDims.width;
	int h = inimg->noDims.height;

	int noBins = histogram->noBins;

	Vector4f *inimg_ptr = inimg->GetData(MEMORYDEVICE_CUDA);
	Vector4f* ptcloud_ptr = ptcloud_out->GetData(MEMORYDEVICE_CUDA);
	float* histogram_ptr = histogram->getPosteriorHistogram(true);

	dim3 blockSize(16, 16);
	dim3 gridSize((int)ceil((float)w / (float)blockSize.x), (int)ceil((float)h / (float)blockSize.y));

	preparePointCloudFromAlignedRGBDImage_device << <gridSize, blockSize >> >(ptcloud_ptr, inimg_ptr, histogram_ptr, intrinsic, boundingbox, inimg->noDims, noBins);
}

void LibISR::Engine::ISRLowlevelEngine_GPU::computepfImageFromHistogram(UChar4Image *rgb_in, Objects::ISRHistogram *histogram)
{
	
	int w = rgb_in->noDims.width;
	int h = rgb_in->noDims.height;

	int noBins = histogram->noBins;

	Vector4u *inimg_ptr = rgb_in->GetData(MEMORYDEVICE_CUDA);
	float* histogram_ptr = histogram->getPosteriorHistogram(true);

	dim3 blockSize(16, 16);
	dim3 gridSize((int)ceil((float)w / (float)blockSize.x), (int)ceil((float)h / (float)blockSize.y));

	computepfImageFromHistogram_device << <gridSize, blockSize >> >(inimg_ptr, histogram_ptr, rgb_in->noDims, noBins);
	rgb_in->UpdateHostFromDevice();
}


//////////////////////////////////////////////////////////////////////////
// device functions
//////////////////////////////////////////////////////////////////////////

__global__ void subsampleImageRGBDImage_device(const Vector4f *imageData_in, Vector2i oldDims, Vector4f *imageData_out, Vector2i newDims)
{
	int x = threadIdx.x + blockIdx.x * blockDim.x, y = threadIdx.y + blockIdx.y * blockDim.y;

	if (x > newDims.x - 1 || y > newDims.y - 1) return;

	filterSubsampleWithHoles(imageData_out, x, y, newDims, imageData_in, oldDims);
}

__global__ void prepareAlignedRGBDData_device(Vector4f* rgbd_out, const short *depth_in, const Vector4u *rgb_in, const Vector2i imgSize, Matrix3f H, Vector3f T)
{
	int x = threadIdx.x + blockIdx.x * blockDim.x, y = threadIdx.y + blockIdx.y * blockDim.y;
	if (x > imgSize.x - 1 || y > imgSize.y - 1) return;

	int idx = y * imgSize.x + x;
	ushort rawdepth = (ushort)depth_in[idx];
	float z = rawdepth == 65535 ? 0 : ((float)rawdepth / 1000.0f);

	// Vector3f uv_depth = (y, x, 1.0f);
	// Vector3f uv_color = z / 1000.0f * H * uv_depth + T / 1000.0f;

	// int X = static_cast<int>(uv_color[0] / uv_color[2]);
	// int Y = static_cast<int>(uv_color[1] / uv_color[2]);
	// int idx_depth_to_rgb = Y * 1920 + X;

	// if((X >= 0 && X < 1920) && (Y >= 0 && Y < 1080)){
	// 	rgbd_out[idx].x = rgb_in_ptr[idx_depth_to_rgb].r;
	// 	rgbd_out[idx].y = rgb_in_ptr[idx_depth_to_rgb].g;
	// 	rgbd_out[idx].z = rgb_in_ptr[idx_depth_to_rgb].b;
	// 	rgbd_out[idx].w = z;
	// }else{
	// 	rgbd_out[idx].x = 0;
	// 	rgbd_out[idx].y = 0;
	// 	rgbd_out[idx].z = 0;
	// 	rgbd_out[idx].w = z;
	// }


	if (T.x == 0 && T.y == 0 && T.z == 0)
	{
		rgbd_out[idx].x = rgb_in[idx].r;
		rgbd_out[idx].y = rgb_in[idx].g;
		rgbd_out[idx].z = rgb_in[idx].b;
		rgbd_out[idx].w = z;
		return;
	}
	rgbd_out[idx].w = z;
	mapRGBDtoRGB(rgbd_out[idx], Vector3f(x*z, y*z, z), rgb_in, imgSize, H, T);
}

__global__ void preparePointCloudFromAlignedRGBDImage_device(Vector4f* ptcloud_out, Vector4f* inimg, float* histogram, Vector4f intrinsic, Vector4i boundingbox, Vector2i imgSize, int histBins)
{
	int x = threadIdx.x + blockIdx.x * blockDim.x, y = threadIdx.y + blockIdx.y * blockDim.y;
	if (x > imgSize.x - 1 || y > imgSize.y - 1) return;
	
	int idx = y * imgSize.x + x;

	if (x < boundingbox.x || x >= boundingbox.z || y < boundingbox.y || y >= boundingbox.w)
	{ 
		ptcloud_out[idx] = Vector4f(0, 0, 0, -1);
	}
	else
	{
		float z = inimg[idx].w;
		unprojectPtWithIntrinsic(intrinsic, Vector3f(x*z, y*z, z), ptcloud_out[idx]);

		ptcloud_out[idx].w = getPf(inimg[idx], histogram, histBins);
	}
}

__global__ void computepfImageFromHistogram_device(Vector4u* inimg, float* histogram, Vector2i imgSize, int histBins)
{
	int x = threadIdx.x + blockIdx.x * blockDim.x, y = threadIdx.y + blockIdx.y * blockDim.y;
	if (x > imgSize.x - 1 || y > imgSize.y - 1) return;

	int idx = y * imgSize.x + x;
	float pf = getPf(inimg[idx], histogram, histBins);

	if (pf>0.5f)
	{
		inimg[idx].r = 255;
		inimg[idx].g = 0;
		inimg[idx].b = 0;
	}
	else if (pf==0.5f)
	{
		inimg[idx].r = 0;
		inimg[idx].g = 0;
		inimg[idx].b = 255;
	}

}

