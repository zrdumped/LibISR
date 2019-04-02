#pragma once
#include "../../LibISRDefine.h"

using namespace ORUtils;

namespace LibISR
{
	namespace Engine
	{
		class MatrixAssist
		{
		public:
            static void Resize(const Vector4<unsigned char> *src, Vector4<unsigned char>* dst, 
				Vector2<int> srcSize, Vector2<int> dstSize);
        
            void Test();

            MatrixAssist(){}
            ~MatrixAssist(){}
			
		};

	}

}