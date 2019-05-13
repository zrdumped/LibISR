// Copyright 2014-2015 Isis Innovation Limited and the authors of LibISR

#pragma once
#include "../LibISRDefine.h"


namespace LibISR
{
	namespace Objects
	{
		/** \brief
		Represents the extrinsic calibration between RGB and depth
		cameras
		*/
		class ISRExtrinsics
		{
		public:
			/** The transformation matrix representing the
			extrinsic calibration data.
			*/
			Matrix4f calib;
			/** Inverse of the above. */
			Matrix4f calib_inv;

			Matrix3f R;
			Vector3f T;

			/** Setup from a given 4x4 matrix, where only the upper
			three rows are used. More specifically, m00...m22
			are expected to contain a rotation and m30...m32
			contain the translation.
			*/

			void SetFrom(const Matrix4f & src)
			{
				this->calib = src;
				for(int i = 0; i < 3; i++){
					for(int j = 0; j < 3; j++){
						this->R.m[i * 3 + j] = src.m[i * 4 + j];
					}
				}
				for(int i = 0; i < 3; i++){
					this->T.v[i] = src.m[i * 4 + 3];
				}
				this->calib_inv.setIdentity();
				for (int r = 0; r < 3; ++r) for (int c = 0; c < 3; ++c) this->calib_inv.m[r + 4 * c] = this->calib.m[c + 4 * r];
				for (int r = 0; r < 3; ++r) {
					float & dest = this->calib_inv.m[r + 4 * 3];
					dest = 0.0f;
					for (int c = 0; c < 3; ++c) dest -= this->calib.m[c + 4 * r] * this->calib.m[c + 4 * 3];
				}
			}

			ISRExtrinsics()
			{
				Matrix4f m;
				m.setZeros();
				m.m00 = m.m11 = m.m22 = m.m33 = 1.0;
				SetFrom(m);
			}
		};
	}
}
