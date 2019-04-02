# LibISR: Implicit Shape Representation for 3D Tracking and Reconstruction

## My Modifications and TODO list

1. The original version only supports KinectV1 running on openni2. It could not get images with right resolution (RGB 1920x1080, Depth 512x424) from kinectv2. I implemented a new interface using Libfreenect2 to solve this problem.

2. Based on the kinectV1, the code assumes that rgb and depth images are with the same resolution (640x480), causing lots of bugs which would be triggered once their resolutions are different. Besides, some functions like display are also implemented based on the same resolution, I am working on it.

3. [TODO] Add different and more objects into the tracking system

4. [TODO] Optimize the code for GPU - friendly.

5. [TODO] Receive and send pose information.

## Original ReadMe is below

This is the software bundle "LibISR",  the current version is maintained by:

  [Carl Yuheng Ren](http://www.carlyuheng.com/) <<carl@robots.ox.ac.uk>>  
  [Victor Adrian Prisacariu](http://www.robots.ox.ac.uk/~victor/) <<victor@robots.ox.ac.uk>> 
  
For more information about LibISR please visit the project website http://www.robots.ox.ac.uk/~victor/libisr.

Other related projects can be found in the Oxford Active Vision Library http://www.oxvisionlib.org.

## Requirements:

  - cmake (e.g. version 2.8.10.2 or 3.2.3)  
    REQUIRED for Linux, unless you write your own build system  
    OPTIONAL for MS Windows, if you use MSVC instead  
    available at http://www.cmake.org/

  - OpenGL / GLUT (e.g. freeglut 2.8.0 or 3.0.0)  
    REQUIRED for the visualisation  
    the library should run without  
    available at http://freeglut.sourceforge.net/

  - CUDA (e.g. version 6.0 or 7.0)  
    REQUIRED for all GPU accelerated code  
    at least with cmake it is still possible to compile the CPU part without  
    available at https://developer.nvidia.com/cuda-downloads

  - OpenNI (e.g. version 2.2.0.33)  
    REQUIRED to get live images from suitable hardware  
    also make sure you have freenect/OpenNI2-FreenectDriver if you need it  
    available at http://structure.io/openni

##Build Process

  To compile the system, use the standard cmake approach:

```
  $ mkdir build
  $ cd build
  $ cmake .. -DOPEN_NI_ROOT=/path/to/OpenNI2/
  $ make
```

## Run Demo

```
  $ ./demo ../Data/teacan.bin ../Data/Calib_reg.txt
```
then you can play with a coke can as shown in this [video (on Yutube)](https://www.youtube.com/watch?v=ExAqnnEZOVU&feature=youtu.be) or [video (on Weibo)](http://video.weibo.com/show?fid=1034:50e519c5d1d8974a02dc6ace742910ca)

## Citation

If you use this code for your research, please kindly cite the following publications:
```
@Inproceedings{Ren_3DV_2014,
author={Ren, C.Y. and Prisacariu, V. and Kaehler, O. and Reid, I. and Murray, D.},
booktitle={3D Vision (3DV), 2014 2nd International Conference on},
title={3D Tracking of Multiple Objects with Identical Appearance Using RGB-D Input},
year={2014},
month={Dec},
volume={1},
pages={47-54}
}

```

```
@Inproceedings{star3d_iccv_2013, 
author={Ren, C.Y. and Prisacariu, V. and Murray, D. and Reid, I.}, 
booktitle={Computer Vision (ICCV), 2013 IEEE International Conference on}, 
title={STAR3D: Simultaneous Tracking and Reconstruction of 3D Objects Using RGB-D Data}, 
year={2013}, 
month={Dec}, 
pages={1561-1568}, 
}

```
