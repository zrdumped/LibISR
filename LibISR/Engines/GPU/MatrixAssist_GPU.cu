#include "MatrixAssist_GPU.h"
#include <stdint.h>

using namespace LibISR::Engine;



__global__ void cudaTransform_device(uint8_t *output, uint8_t *input, uint32_t pitchOutput, uint32_t pitchInput, uint8_t bytesPerPixelOutput, uint8_t bytesPerPixelInput, float xRatio, float yRatio);


void LibISR::Engine::MatrixAssist::Test(){
    return;
}

//1920x1080 -> 640x480
void LibISR::Engine::MatrixAssist::Resize(const Vector4<unsigned char> *src, Vector4<unsigned char>* dst, 
				Vector2<int> srcSize, Vector2<int> dstSize)
{
    uint32_t src_row_btyes;
    uint32_t dst_row_bytes;
    int src_nb_component;
    int dst_nb_component;
    uint32_t src_size;
    uint32_t dst_size;
    uint8_t* device_src;
    uint8_t* device_dst;

    float x_ratio = dstSize.x / srcSize.x;
    float y_ratio = dstSize.y / srcSize.y;

    dim3 grid(dstSize.x, dstSize.y);
	dim3 blockSize(1, 1);

    src_row_btyes = (srcSize.x * 3 + 3) & ~3;
    dst_row_bytes = (dstSize.x * 4 + 3) & ~3;

    src_nb_component = 3;
    dst_nb_component = 4;
    src_size = srcSize.x * srcSize.y;
    dst_size = dstSize.x * dstSize.y;

    // Copy original image
    ORcudaSafeCall(cudaMalloc((void **)&device_src, src_size));
    ORcudaSafeCall(cudaMemcpy(device_src, src, src_size, cudaMemcpyHostToDevice));
    ORcudaSafeCall(cudaMalloc((void **)&device_dst, dst_size));
    cudaTransform_device << < grid, blockSize >> >(device_dst, device_src, dst_row_bytes, src_row_btyes, dst_nb_component, src_nb_component, x_ratio, y_ratio);
    
	// Copy scaled image to host
    ORcudaSafeCall(cudaMemcpy(dst, device_dst, dst_size, cudaMemcpyDeviceToHost));
    ORcudaSafeCall(cudaFree(device_src));
    ORcudaSafeCall(cudaFree(device_dst));
}

__global__ void cudaTransform_device(uint8_t *output, uint8_t *input, uint32_t pitchOutput, uint32_t pitchInput, uint8_t bytesPerPixelOutput, uint8_t bytesPerPixelInput, float xRatio, float yRatio)
{
	int x = (int)(xRatio * blockIdx.x);
	int y = (int)(yRatio * blockIdx.y);

	uint8_t *a; uint8_t *b; uint8_t *c; uint8_t *d;
	float xDist, yDist, blue, red, green;

	// X and Y distance difference
	xDist = (xRatio * blockIdx.x) - x;
	yDist = (yRatio * blockIdx.y) - y;

	// Points
	a = input + y * pitchInput + x * bytesPerPixelInput;
	b = input + y * pitchInput + (x + 1) * bytesPerPixelInput;
	c = input + (y + 1) * pitchInput + x * bytesPerPixelInput;
	d = input + (y + 1) * pitchInput + (x + 1) * bytesPerPixelInput;

	// blue
	blue = (a[2])*(1 - xDist)*(1 - yDist) + (b[2])*(xDist)*(1 - yDist) + (c[2])*(yDist)*(1 - xDist) + (d[2])*(xDist * yDist);

	// green
	green = ((a[1]))*(1 - xDist)*(1 - yDist) + (b[1])*(xDist)*(1 - yDist) + (c[1])*(yDist)*(1 - xDist) + (d[1])*(xDist * yDist);

	// red
	red = (a[0])*(1 - xDist)*(1 - yDist) + (b[0])*(xDist)*(1 - yDist) + (c[0])*(yDist)*(1 - xDist) + (d[0])*(xDist * yDist);

	uint8_t *p = output + blockIdx.y * pitchOutput + blockIdx.x * bytesPerPixelOutput;
	*(uint32_t*)p = 0xff000000 | ((((int)blue) << 16)) | ((((int)green) << 8)) | ((int)red);
}
