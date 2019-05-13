// Copyright 2014-2015 Isis Innovation Limited and the authors of LibISR

#pragma once

#include "../../LibISRDefine.h"

_CPU_AND_GPU_CODE_ inline void unprojectPtWithIntrinsic(const Vector4f intrinsic, const Vector3f &inpt, Vector4f &outpt)
{
	outpt.x = (inpt.x - inpt.z*intrinsic.z) / intrinsic.x;
	outpt.y = (inpt.y - inpt.z*intrinsic.w) / intrinsic.y;
	outpt.z = inpt.z;
	outpt.w = 1.0f;
}

template<class T>
_CPU_AND_GPU_CODE_ inline float getPf(const T &pixel, float* histogram, int noBins)
{
	int dim = noBins*noBins*noBins;

	int ru = pixel.r / noBins;
	int gu = pixel.g / noBins;
	int bu = pixel.b / noBins;

	int pidx = ru*noBins*noBins + gu * noBins + bu;

	return histogram[pidx];
}

template<class VecType>
_CPU_AND_GPU_CODE_ inline void mapRGBDtoRGB(VecType &rgb_out, const Vector3f& inpt, const Vector4u *rgb_in, const Vector2i& imgSize, const Matrix3f &H, const Vector3f &T)
{
	//printf("[mapRGBDtoRGB]\n");
	if (inpt.z>0)
	{
		Vector3f imgPt = H*inpt + T;
		int ix = (int)(imgPt.x / imgPt.z);
		int iy = (int)(imgPt.y / imgPt.z);

		//printf("%d %d %f\n", ix, iy, inpt.z);

		if (ix >= 0 && ix < imgSize.x && iy >= 0 && imgSize.y)
		{
			rgb_out.x = rgb_in[iy * imgSize.x + ix].x;
			rgb_out.y = rgb_in[iy * imgSize.x + ix].y;
			rgb_out.z = rgb_in[iy * imgSize.x + ix].z;

			return;
		}
	}
	rgb_out.r = rgb_out.g = rgb_out.b = 0;
}

template<class VecType, class VecType2>
_CPU_AND_GPU_CODE_ inline void mapRGBDtoRGB(VecType &rgb_out, VecType2 &aligned_rgb, const Vector3f& inpt, const Vector4u *rgb_in, const Vector2i& imgSize, const Matrix3f &H, const Vector3f &T)
{
	if (inpt.z>0)
	{
		Vector3f imgPt = H*inpt + T;
		int ix = (int)(imgPt.x / imgPt.z);
		int iy = (int)(imgPt.y / imgPt.z);

		if (ix >= 0 && ix < imgSize.x && iy >= 0 && imgSize.y)
		{
			rgb_out.x = rgb_in[iy * imgSize.x + ix].x;
			rgb_out.y = rgb_in[iy * imgSize.x + ix].y;
			rgb_out.z = rgb_in[iy * imgSize.x + ix].z;
			aligned_rgb.x = rgb_in[iy * imgSize.x + ix].x;
			aligned_rgb.y = rgb_in[iy * imgSize.x + ix].y;
			aligned_rgb.z = rgb_in[iy * imgSize.x + ix].z;
			//printf("Pos %d %d, RGB: %d %d %d\n",
			//ix, iy, rgb_in[iy * imgSize.x + ix].x, 
			//rgb_in[iy * imgSize.x + ix].y, rgb_in[iy * imgSize.x + ix].z);
			return;
		}
	}
	rgb_out.r = rgb_out.g = rgb_out.b = 0;
	aligned_rgb.r = aligned_rgb.g = aligned_rgb.b = 0;
}


_CPU_AND_GPU_CODE_ inline void filterSubsampleWithHoles(Vector4f *imageData_out, int x, int y, Vector2i newDims, const Vector4f *imageData_in, Vector2i oldDims)
{
	int src_pos_x = x * 2, src_pos_y = y * 2;
	Vector4f pixel_out = 0.0f, pixel_in; float no_good_pixels = 0.0f;

	pixel_in = imageData_in[(src_pos_x + 0) + (src_pos_y + 0) * oldDims.x];
	pixel_out.x += pixel_in.x; pixel_out.y += pixel_in.y; pixel_out.z += pixel_in.z;
	if (pixel_in.w > 0) { pixel_out.w += pixel_in.w; no_good_pixels++; }

	pixel_in = imageData_in[(src_pos_x + 1) + (src_pos_y + 0) * oldDims.x];
	pixel_out.x += pixel_in.x; pixel_out.y += pixel_in.y; pixel_out.z += pixel_in.z;
	if (pixel_in.w > 0) { pixel_out.w += pixel_in.w; no_good_pixels++; }

	pixel_in = imageData_in[(src_pos_x + 0) + (src_pos_y + 1) * oldDims.x];
	pixel_out.x += pixel_in.x; pixel_out.y += pixel_in.y; pixel_out.z += pixel_in.z;
	if (pixel_in.w > 0) { pixel_out.w += pixel_in.w; no_good_pixels++; }

	pixel_in = imageData_in[(src_pos_x + 1) + (src_pos_y + 1) * oldDims.x];
	pixel_out.x += pixel_in.x; pixel_out.y += pixel_in.y; pixel_out.z += pixel_in.z;
	if (pixel_in.w > 0) { pixel_out.w += pixel_in.w; no_good_pixels++; }

	pixel_out.x /= 4; pixel_out.y /= 4; pixel_out.z /= 4;

	if (no_good_pixels > 0) pixel_out.w /= no_good_pixels;
	else { pixel_out.w = -1.0f; }
	
	imageData_out[x + y * newDims.x] = pixel_out;
}
