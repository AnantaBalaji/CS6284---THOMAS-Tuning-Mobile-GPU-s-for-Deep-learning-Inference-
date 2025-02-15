// Copyright Olivier Valery 2017 olivier.valery92 at gmail.com
// this work is based on DeepCL: a project of Hugh Perkins hughperkins at gmail
//
// This Source Code Form is subject to the terms of the Mozilla Public License, 
// v. 2.0. If a copy of the MPL was not distributed with this file, You can 
// obtain one at http://mozilla.org/MPL/2.0/.

#include <cstring>
#include <stdexcept>
using namespace std;

#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
//#ifdef USE_CLEW
//#include "thirdparty/clew/include/clew.h"
//#else
#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
//#endif
#include "../src/dependencies.h"
#include "EasyCL.h"

//olivier: no half in this framework
//#include "CLHalfWrapper.h"
#include "CLArrayFloat.h"
#include "CLArrayInt.h"
#include "CLIntWrapper.h"
#include "CLFloatWrapperConst.h"
#include "CLWrapper.h"
#include "CLKernel.h"
#include "util/easycl_stringhelper.h"
#include <fstream>

EasyCL::EasyCL(int gpu, bool verbose) {
    init(gpu, verbose);
}
EasyCL::EasyCL(int gpu) {
    init(gpu, true);
}

EasyCL::EasyCL(bool verbose) {
     init(0, verbose);
}
EasyCL::EasyCL() {
     init(0, true);
}

void EasyCL::commonConstructor(cl_platform_id platform_id, cl_device_id device, bool verbose) {
    this->verbose = verbose;
    queue = 0;
    context = 0;
    profilingOn = false;

//    #ifdef USE_CLEW
//        bool clpresent = 0 == clewInit();
//        if(!clpresent) {
//            throw std::runtime_error("OpenCL library not found");
//        }
//    #endif
    this->platform_id = platform_id;
    this->device = device;

    if(verbose) {
        std::cout << "Using " << getPlatformInfoString(platform_id, CL_PLATFORM_VENDOR) << " , OpenCL platform: " << getPlatformInfoString(platform_id, CL_PLATFORM_NAME) << std::endl;
    //    std::cout << "Using " << getPlatformInfoString(platform_id, CL_PLATFORM_NAME) << std::endl;
        std::cout << "Using OpenCL device: " << getDeviceInfoString(device, CL_DEVICE_NAME) << std::endl;
    }

    // Context
    context = new cl_context();
    *context = clCreateContext(0, 1, &device, NULL, NULL, &error);
    if (error != CL_SUCCESS) {
       throw std::runtime_error("Error creating OpenCL context, OpenCL errocode: " + errorMessage(error));
    }
    // Command-queue
    queue = new cl_command_queue;
    *queue = clCreateCommandQueue(*context, device, 0, &error);
    if (error != CL_SUCCESS) {
       throw std::runtime_error("Error creating OpenCL command queue, OpenCL errorcode: " + errorMessage(error));
    }
}

EasyCL::EasyCL(cl_platform_id platform_id, cl_device_id device, bool verbose) {
    commonConstructor(platform_id, device, verbose);
}

EasyCL::EasyCL(cl_platform_id platform_id, cl_device_id device) {
    commonConstructor(platform_id, device, true);
}
EasyCL *EasyCL::createForFirstGpu() {
//  cout << "createForFirstgpu" << endl;
    return createForIndexedGpu(0, true);
}
EasyCL *EasyCL::createForFirstGpu(bool verbose) {
//  cout << "createForFirstgpu verbose=" << verbose << endl;
    return createForIndexedGpu(0, verbose);
}
EasyCL *EasyCL::createForFirstGpuOtherwiseCpu() {
    return createForFirstGpuOtherwiseCpu(true);
}
EasyCL *EasyCL::createForFirstGpuOtherwiseCpu(bool verbose) {
    try {
        return createForIndexedGpu(0, verbose);
    } catch(std::runtime_error error) {
#if DEEPCL_VERBOSE == 1
    	LOGI( "Couldnt find OpenCL-enabled GPU");
#endif

        cout << "Couldnt find OpenCL-enabled GPU: " << error.what() << endl;
        cout << "Trying for OpenCL-enabled CPU" << endl;
    }
    return createForPlatformDeviceIndexes(0, 0);
}

EasyCL *EasyCL::createForIndexedGpu(int gpu) {
//  cout << "createForIndexedgpu gpu=" << gpu << endl;
    return createForIndexedGpu(gpu, true);
}

EasyCL *EasyCL::createForIndexedDevice(int device) {
    return createForIndexedDevice(device, true);
}

EasyCL *EasyCL::createForIndexedGpu(int gpu, bool verbose) {
//  cout << "createForindexedgpu gpu=" << gpu << " verbose=" << verbose << endl;
//    #ifdef USE_CLEW
//        bool clpresent = 0 == clewInit();
//        if(!clpresent) {
//            throw std::runtime_error("OpenCL library not found");
//        }
//    #endif
#if DEEPCL_VERBOSE == 1
	LOGI( "createForIndexedGpu");
#endif

    cl_int error;
    int currentGpuIndex = 0;
    cl_platform_id platform_ids[10];
    cl_uint num_platforms;
    error = clGetPlatformIDs(10, platform_ids, &num_platforms);
    if (error != CL_SUCCESS) {
#if DEEPCL_VERBOSE == 1
    	LOGI( "Error getting OpenCL platforms ids");
#endif

       throw std::runtime_error("Error getting OpenCL platforms ids, OpenCL errorcode: " + errorMessage(error));
    }
    if(num_platforms == 0) {
#if DEEPCL_VERBOSE == 1
    	LOGI( "Error: no OpenCL platforms available");
#endif

       throw std::runtime_error("Error: no OpenCL platforms available");
    }
    for(int platform =  0; platform < (int)num_platforms; platform++) {
        cl_platform_id platform_id = platform_ids[platform];
#if DEEPCL_VERBOSE == 1
        LOGI( "platform_id ");
#endif

//        cout << "checking platform id " << platform_id << endl;
        cl_device_id device_ids[100];
        cl_uint num_devices;
#if DEEPCL_VERBOSE == 1
        LOGI( "1");
#endif

        //olivier : I delete "CL_DEVICE_TYPE_ACCELERATOR" (no dedicate "Dedicated OpenCL accelerators")
        //in error = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR , 100, device_ids, &num_devices);
        //note: on our mobile device, if the program search the opencl accelerator, it will crash imediatly
        error = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU  , 100, device_ids, &num_devices);
#if DEEPCL_VERBOSE == 1
        string buildLogMessage = std::to_string(num_devices) + "\n";
        LOGI("num_devices: %s", buildLogMessage.c_str());
#endif

        if (error != CL_SUCCESS) {
#if DEEPCL_VERBOSE == 1
        	LOGI( "Error getting device ids for platform");
#endif

            continue;
//           throw std::runtime_error("Error getting device ids for platform " + toString(platform) + ": " + errorMessage(error));
        }
//        cout << "gpu=" << gpu << " currentGpuIndex=" << currentGpuIndex << " num_devices=" << num_devices << endl;
        if(( gpu - currentGpuIndex) < (int)num_devices) {
            return new EasyCL(platform_id, device_ids[(gpu - currentGpuIndex)], verbose);
        } else {
            currentGpuIndex += num_devices;
        }
    }
    if(gpu == 0) {
#if DEEPCL_VERBOSE == 1
    	LOGI( "No OpenCL-enabled GPUs found");
#endif

        throw std::runtime_error("No OpenCL-enabled GPUs found");
    } else {
#if DEEPCL_VERBOSE == 1
    	LOGI( "Not enough OpenCL-enabled GPUs found to satisfy gpu index");
#endif

        throw std::runtime_error("Not enough OpenCL-enabled GPUs found to satisfy gpu index: " + toString(gpu) );
    }
}

EasyCL *EasyCL::createForIndexedDevice(int device, bool verbose) {
//    #ifdef USE_CLEW
//        bool clpresent = 0 == clewInit();
//        if(!clpresent) {
//            throw std::runtime_error("OpenCL library not found");
//        }
//    #endif
    cl_int error;
    int currentGpuIndex = 0;
    cl_platform_id platform_ids[10];
    cl_uint num_platforms;
    error = clGetPlatformIDs(10, platform_ids, &num_platforms);
    if (error != CL_SUCCESS) {
       throw std::runtime_error("Error getting OpenCL platforms ids, OpenCL errorcode: " + errorMessage(error));
    }
    if(num_platforms == 0) {
       throw std::runtime_error("Error: no OpenCL platforms available");
    }
    for(int platform =  0; platform < (int)num_platforms; platform++) {
        cl_platform_id platform_id = platform_ids[platform];
//        cout << "checking platform id " << platform_id << endl;
        cl_device_id device_ids[100];
        cl_uint num_devices;
        error = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU , 100, device_ids, &num_devices);
        if (error != CL_SUCCESS) {
            continue;
//           throw std::runtime_error("Error getting device ids for platform " + toString(platform) + ": " + errorMessage(error));
        }
        if(( device - currentGpuIndex) < (int)num_devices) {
            return new EasyCL(platform_id, device_ids[(device - currentGpuIndex)], verbose);
        } else {
            currentGpuIndex += num_devices;
        }
    }
    if(device == 0) {
        throw std::runtime_error("No OpenCL devices found");
    } else {
        throw std::runtime_error("Not enough OpenCL devices found to satisfy gpu index: " + toString(device) );
    }
}

EasyCL *EasyCL::createForPlatformDeviceIndexes(int platformIndex, int deviceIndex) {
//    #ifdef USE_CLEW
//        bool clpresent = 0 == clewInit();
//        if(!clpresent) {
//            throw std::runtime_error("OpenCL library not found");
//        }
//    #endif
    cl_int error;
//    int currentGpuIndex = 0;
    cl_platform_id platform_ids[10];
    cl_uint num_platforms;
    error = clGetPlatformIDs(10, platform_ids, &num_platforms);
    if (error != CL_SUCCESS) {
       throw std::runtime_error("Error getting OpenCL platforms ids, OpenCL errorcode: " + errorMessage(error));
    }
    if(num_platforms == 0) {
       throw std::runtime_error("Error: no OpenCL platforms available");
    }
    if(platformIndex >= (int)num_platforms) {
       throw std::runtime_error("Error: OpenCL platform index " + toString(platformIndex) + " not available. There are only: " + toString(num_platforms) + " platforms available");
    }
    cl_platform_id platform_id = platform_ids[platformIndex];
    cl_device_id device_ids[100];
    cl_uint num_devices;
    error = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_CPU, 100, device_ids, &num_devices);
    if (error != CL_SUCCESS) {
       throw std::runtime_error("Error getting OpenCL device ids for platform index " + toString(platformIndex) + ": OpenCL errorcode: " + errorMessage(error));
    }
    if(num_devices == 0) {
       throw std::runtime_error("Error: no OpenCL devices available for platform index " + toString(platformIndex) );
    }
    if(deviceIndex >= (int)num_devices) {
       throw std::runtime_error("Error: OpenCL device index " + toString(deviceIndex) + " goes beyond the available devices on platform index " + toString(platformIndex) + ", which has " + toString(num_devices) + " devices");
    }
    return new EasyCL(platform_id, device_ids[deviceIndex]);
}

EasyCL *EasyCL::createForPlatformDeviceIds(cl_platform_id platformId, cl_device_id deviceId) {
    return new EasyCL(platformId, deviceId);
}

void EasyCL::init(int gpuIndex, bool verbose) {
//    #ifdef USE_CLEW
//        bool clpresent = 0 == clewInit();
//        if(!clpresent) {
//            throw std::runtime_error("OpenCL library not found");
//        }
//    #endif

//        std::cout << "this: " << this << std::endl;
//        this->gpuIndex = gpuindex;
    error = 0;

    queue = 0;
    context = 0;

    // Platform
    cl_uint num_platforms;
    error = clGetPlatformIDs(1, &platform_id, &num_platforms);
//        std::cout << "num platforms: " << num_platforms << std::endl;
//        assert (num_platforms == 1);
    if (error != CL_SUCCESS) {
       throw std::runtime_error("Error getting OpenCL platforms ids: " + errorMessage(error));
    }
    if(num_platforms == 0) {
       throw std::runtime_error("Error: no OpenCL platforms available");
    }

    cl_uint num_devices;
    error = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR , 0, 0, &num_devices);
    if (error != CL_SUCCESS) {
       throw std::runtime_error("Error getting OpenCL device ids: " + errorMessage(error));
    }
//      std::cout << "num devices: " << num_devices << std::endl;
    cl_device_id *device_ids = new cl_device_id[num_devices];
    error = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR, num_devices, device_ids, &num_devices);
    if (error != CL_SUCCESS) {
       throw std::runtime_error("Error getting OpenCL device ids: " + errorMessage(error));
    }

    if(gpuIndex >= static_cast<int>(num_devices) ) {
       throw std::runtime_error("requested gpuindex " + toString(gpuIndex) + " goes beyond number of available device " + toString(num_devices) );
    }
    device = device_ids[gpuIndex];
    delete[] device_ids;

    // Context
    context = new cl_context();
    *context = clCreateContext(0, 1, &device, NULL, NULL, &error);
    if (error != CL_SUCCESS) {
       throw std::runtime_error("Error creating OpenCL context, OpenCL errorcode: " + errorMessage(error));
    }
    // Command-queue
    queue = new cl_command_queue;
    *queue = clCreateCommandQueue(*context, device, 0, &error);
    if (error != CL_SUCCESS) {
       throw std::runtime_error("Error creating OpenCL command queue, OpenCL errorcode: " + errorMessage(error));
    }
}

EasyCL::~EasyCL() {
    for(map< string, bool >::iterator it = kernelOwnedByName.begin(); it != kernelOwnedByName.end(); it++) {
        if(it->second) {
            delete kernelByName[ it->first ];
        }
    }

    for(vector< cl_event * >::iterator it = profilingEvents.begin(); it != profilingEvents.end(); it++) {
      clReleaseEvent(**it);
      delete *it;
    }

//        clReleaseProgram(program);
    if(queue != 0) {
//        cout << "releasing OpenCL command queue" << endl;
        clReleaseCommandQueue(*queue);
        delete queue;
    }
    if(context != 0) {
//        cout << "releasing OpenCL context" << endl;
        clReleaseContext(*context);
        delete context;
    }
}

CLArrayFloat *EasyCL::arrayFloat(int N) {
    return new CLArrayFloat(N, this);
}

CLArrayInt *EasyCL::arrayInt(int N) {
    return new CLArrayInt(N, this);
}

CLIntWrapper *EasyCL::wrap(int N, int *source) {
    return new CLIntWrapper(N, source, this);
}

CLUCharWrapper *EasyCL::wrap(int N, unsigned char*source) {
    return new CLUCharWrapper(N, source, this);
}

CLFloatWrapper *EasyCL::wrap(int N, float *source) {
    return new CLFloatWrapper(N, source, this);
}

CLFloatWrapperConst *EasyCL::wrap(int N, float const*source) {
    return new CLFloatWrapperConst(N, source, this);
}

CLKernel *EasyCL::buildKernel(string kernelfilepath, string kernelname) {
    return buildKernel(kernelfilepath, kernelname, "");
}

//CLHalfWrapper *EasyCL::wrap(int N, half *source) {
//	//olivier
//    return new CLHalfWrapper(N, source, this);
//}

CLKernel *EasyCL::buildKernel(string kernelfilepath, string kernelname, string options) {
    std::string path = kernelfilepath.c_str();
    std::string source = getFileContents(path);
    return buildKernelFromString(source, kernelname, options, kernelfilepath);
}

bool SaveProgramBinary(cl_program program, cl_device_id device, const char* fileName)
{
    cl_uint numDevices = 0;
    cl_int errNum;

    // 1 - Query for number of devices attached to program
    errNum = clGetProgramInfo(program, CL_PROGRAM_NUM_DEVICES, sizeof(cl_uint),
                              &numDevices, NULL);
    if (errNum != CL_SUCCESS)
    {
        LOGE( "Error querying for number of devices." );
        return false;
    }

    // 2 - Get all of the Device IDs
    cl_device_id *devices = new cl_device_id[numDevices];
    errNum = clGetProgramInfo(program, CL_PROGRAM_DEVICES,
                              sizeof(cl_device_id) * numDevices,
                              devices, NULL);
    if (errNum != CL_SUCCESS)
    {
    	LOGE( "Error querying for devices.");
        delete [] devices;
        return false;
    }

    // 3 - Determine the size of each program binary
    size_t *programBinarySizes = new size_t [numDevices];
    errNum = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES,
                              sizeof(size_t) * numDevices,
                              programBinarySizes, NULL);
    if (errNum != CL_SUCCESS)
    {
    	LOGE( "Error querying for program binary sizes.");
        delete [] devices;
        delete [] programBinarySizes;
        return false;
    }

    unsigned char **programBinaries = new unsigned char*[numDevices];
    for (cl_uint i = 0; i < numDevices; i++)
    {
        programBinaries[i] = new unsigned char[programBinarySizes[i]];
    }

    // 4 - Get all of the program binaries
    errNum = clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char*) * numDevices,
                              programBinaries, NULL);
    if (errNum != CL_SUCCESS)
    {
    	 LOGE( "Error querying for program binaries");

        delete [] devices;
        delete [] programBinarySizes;
        for (cl_uint i = 0; i < numDevices; i++)
        {
            delete [] programBinaries[i];
        }
        delete [] programBinaries;
        return false;
    }

    // 5 - Finally store the binaries for the device requested out to disk for future reading.
    for (cl_uint i = 0; i < numDevices; i++)
    {
        // Store the binary just for the device requested.  In a scenario where
        // multiple devices were being used you would save all of the binaries out here.
        if (devices[i] == device)
        {
            FILE *fp = fopen(fileName, "wb");
            fwrite(programBinaries[i], 1, programBinarySizes[i], fp);
            fclose(fp);
            break;
        }
    }

    // Cleanup
    delete [] devices;
    delete [] programBinarySizes;
    for (cl_uint i = 0; i < numDevices; i++)
    {
        delete [] programBinaries[i];
    }
    delete [] programBinaries;
    return true;
}

///
//  Attempt to create the program object from a cached binary.  Note that
//  on first run this will fail because the binary has not yet been created.
//
cl_program CreateProgramFromBinary(cl_context context, cl_device_id device, const char* fileName, const char* options)
{
	//LOGI("CreateProgramFromBinary start %s",fileName);
    FILE *fp = fopen(fileName, "rb");
    if (fp == NULL)
    {
    	LOGE("-------------------------- ERREOR CreateProgramFromBinary %s not found",fileName);
        return NULL;
    }

    // Determine the size of the binary
    size_t binarySize;
    fseek(fp, 0, SEEK_END);
    binarySize = ftell(fp);
    rewind(fp);

    unsigned char *programBinary = new unsigned char[binarySize];
    fread(programBinary, 1, binarySize, fp);
    fclose(fp);

    cl_int errNum = 0;
    cl_program program;
    cl_int binaryStatus;

    program = clCreateProgramWithBinary(context,
                                        1,
                                        &device,
                                        &binarySize,
                                        (const unsigned char**)&programBinary,
                                        &binaryStatus,
                                        &errNum);
    delete [] programBinary;
    if (errNum != CL_SUCCESS)
    {
    	LOGE("Error loading program binary.");
        //std::cerr << "Error loading program binary." << std::endl;
        return NULL;
    }

    if (binaryStatus != CL_SUCCESS)
    {
    	LOGE("Invalid binary for device");
        //std::cerr << "Invalid binary for device" << std::endl;
        return NULL;
    }
//    errNum = clBuildProgram(program, 1, &device, options, NULL, NULL);
    errNum = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (errNum != CL_SUCCESS)
    {
        // Determine the reason for the error
        char buildLog[16384];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                              sizeof(buildLog), buildLog, NULL);

        LOGE("Error in program: %s",buildLog);
        //std::cerr << "Error in program: " << std::endl;
        //std::cerr << buildLog << std::endl;
        clReleaseProgram(program);
        return NULL;
    }
    return program;
}


CLKernel *EasyCL::buildKernelFromString(string operation,string source, string kernelname, string options, string sourcefilename) {


	binariesManager=new ConfigManager(absolutePath);

	bool compiled;
	string filepath="default";
	if (binariesManager->alreadyCompiledKernel(kernelname,options,operation,filepath)){
		compiled=true;
	}else {
		compiled=false;
	}

	if(compiled){
		//LOGI("--------------------------- the kernel has been prebuilt; OpenCL only need to create the program from the binary files ");
		cl_program program = CreateProgramFromBinary(*context, device, filepath.c_str(),options.c_str());

		cl_kernel kernel = clCreateKernel(program, kernelname.c_str(), &error);
		if(error != CL_SUCCESS) {
			vector<std::string> splitSource = easycl::split(source, "\n");
			std::string sourceWithNumbers = "\nkernel source:\n";
			for(int i = 0; i < (int)splitSource.size(); i++) {
				sourceWithNumbers += toString(i + 1) + ": " + splitSource[i] + "\n";
			}
			sourceWithNumbers += "\n";
			std::string exceptionMessage = "";
			switch(error) {
				case -46:
					LOGE("EasyCL::buildKernelFromString: error 46 (opencl)");
					exceptionMessage = sourceWithNumbers + "\nInvalid kernel name, code -46, kernel " + kernelname + "\n";
					break;
				default:
					exceptionMessage = sourceWithNumbers + "\nSomething went wrong with clCreateKernel, OpenCL erorr code " + toString(error) + "\n";
					break;
			}
			cout << "kernel build error:\n" << exceptionMessage << endl;
			throw std::runtime_error(exceptionMessage);
		}
		checkError(error);

		CLKernel *clk=new CLKernel(this, sourcefilename, kernelname, source, program, kernel);

		return clk;
}else{
	//LOGI("--------------------------- the kernel hasn't been prebuilt; we need to build it in OpenCL");





	size_t src_size = 0;

    const char *source_char = source.c_str();
    src_size = strlen(source_char);
//    cl_program program = new cl_program();
    cl_program program = clCreateProgramWithSource(*context, 1, &source_char, &src_size, &error);

    error = clBuildProgram(program, 1, &device, options.c_str(), NULL, NULL);

    char* build_log;
    size_t log_size;
    error = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
    checkError(error);
    build_log = new char[log_size+1];
    error = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, build_log, NULL);
    checkError(error);
    build_log[log_size] = '\0';
    string buildLogMessage = "";
    if(log_size > 2) {
        buildLogMessage = sourcefilename + " build log: "  + "\n" + build_log;
        //LOGI("%s", buildLogMessage.c_str());
    }
    delete[] build_log;
    checkError(error);

    cl_kernel kernel = clCreateKernel(program, kernelname.c_str(), &error);
    if(error != CL_SUCCESS) {
        vector<std::string> splitSource = easycl::split(source, "\n");
        std::string sourceWithNumbers = "\nkernel source:\n";
        for(int i = 0; i < (int)splitSource.size(); i++) {
            sourceWithNumbers += toString(i + 1) + ": " + splitSource[i] + "\n";
        }
        sourceWithNumbers += "\n";
        std::string exceptionMessage = "";
        switch(error) {
            case -46:
            	LOGE("EasyCL::buildKernelFromString: error 46 (opencl) %s",buildLogMessage.c_str());
                exceptionMessage = /*sourceWithNumbers +*/ "\nInvalid kernel name, code -46, kernel " + kernelname + "\n" + buildLogMessage;
                break;
            default:
                exceptionMessage = /*sourceWithNumbers +*/ "\nSomething went wrong with clCreateKernel, OpenCL erorr code " + toString(error) + "\n" + buildLogMessage;
                break;
        }
        LOGE("kernel build error: %s",exceptionMessage.c_str());
        cout << "kernel build error:\n" << exceptionMessage << endl;
        throw std::runtime_error(exceptionMessage);
    }
    checkError(error);


      binariesManager->addNewCompiledKernel(kernelname, options,operation,filepath);



    SaveProgramBinary(program, device, filepath.c_str());

    return new CLKernel(this, sourcefilename, kernelname, source, program, kernel);

	}
	delete binariesManager;
}

bool EasyCL::isOpenCLAvailable() {
//    #ifdef USE_CLEW
//        return 0 == clewInit();
//    #else
        return true; // I guess?
//    #endif
}

int EasyCL::getPower2Upperbound(int value) {
    int upperbound = 1;
    while(upperbound < value) {
        upperbound <<= 1;
    }
    return upperbound;
}

int EasyCL::roundUp(int quantization, int minimum) {
    int size = (minimum / quantization) * quantization;
    if(size < minimum) {
        size += quantization;
    }
    return size;
}

// accidentally created 2 funcftions that do the same thing :-P  but wont remove either,
// in case someone's using that one
int EasyCL::getNextPower2(int value){
    return getPower2Upperbound(value);
} // eg pass in 320, it will return: 512

void EasyCL::gpu(int gpuIndex) {
    if(queue != 0) {
        clReleaseCommandQueue(*queue);
        delete queue;
    }
    if(context != 0) {
        clReleaseContext(*context);
        delete context;
    }

    init(gpuIndex, this->verbose);
}

void EasyCL::finish() {
    error = clFinish(*queue);
    switch(error) {
        case CL_SUCCESS:
            break;
        case -36:
            throw std::runtime_error("Invalid command queue: often indicates out of bounds memory access within kernel");
        default:
            checkError(error);
    }
}
void EasyCL::setProfiling(bool profiling) {
  finish();
  clReleaseCommandQueue(*queue);
//  delete queue;

//  queue = new cl_command_queue;
  *queue = clCreateCommandQueue(*context, device, profiling ? CL_QUEUE_PROFILING_ENABLE : 0, &error);
  if (error != CL_SUCCESS) {
     throw std::runtime_error("Error creating command queue: " + errorMessage(error));
  }
  this->profilingOn = profiling;
}
void EasyCL::pushEvent(std::string name, cl_event *event) {
  profilingNames.push_back(name);
  profilingEvents.push_back(event);
}
void EasyCL::dumpProfiling() {
  map< string, double > timeByKernel;
  for(int i = 0; i < (int)profilingEvents.size(); i++) {
    string name = profilingNames[i];
    cl_event *event = profilingEvents[i];
    clWaitForEvents(1, event);
    cl_ulong start, end;
    clGetEventProfilingInfo(*event, CL_PROFILING_COMMAND_START, sizeof(start), &start, NULL);
    clGetEventProfilingInfo(*event, CL_PROFILING_COMMAND_END, sizeof(end), &end, NULL);
    cl_ulong time_nanos = end - start;
    double time_millis = time_nanos / 1000000.0;
    timeByKernel[name] += time_millis;
    clReleaseEvent(*event);
    delete event;
  }
  for(map< string, double >::iterator it = timeByKernel.begin(); it != timeByKernel.end(); it++) {
    // cout << it->first << " " << it->second << "ms" << endl;
    LOGI("kernel: %s, %fms", it->first.c_str(), it->second);
  }
  profilingNames.clear();
  profilingEvents.clear();
}
int EasyCL::getComputeUnits() {
    return (int)this->getDeviceInfoInt64(CL_DEVICE_MAX_COMPUTE_UNITS);
}
int EasyCL::getLocalMemorySize() {
    return (int)this->getDeviceInfoInt64(CL_DEVICE_LOCAL_MEM_SIZE);
}
int EasyCL::getLocalMemorySizeKB() {
    return (int)(this->getDeviceInfoInt64(CL_DEVICE_LOCAL_MEM_SIZE) / 1024);
}
int EasyCL::getMaxWorkgroupSize() {
    return (int)this->getDeviceInfoInt64(CL_DEVICE_MAX_WORK_GROUP_SIZE);
}
int EasyCL::getMaxAllocSizeMB() {
    return (int)(this->getDeviceInfoInt64(CL_DEVICE_MAX_MEM_ALLOC_SIZE) / 1024 / 1024);
}

std::string EasyCL::errorMessage(cl_int error) {
    return toString(error);
}

void EasyCL::checkError(cl_int error) {
    if(error != CL_SUCCESS) {
        std::string message = toString(error);
        switch(error) {
            case CL_MEM_OBJECT_ALLOCATION_FAILURE:
                message = "CL_MEM_OBJECT_ALLOCATION_FAILURE";
                break;
            case CL_INVALID_ARG_SIZE:
                message = "CL_INVALID_ARG_SIZE";
                break;
            case CL_INVALID_BUFFER_SIZE:
                message = "CL_INVALID_BUFFER_SIZE";
                break;
        }
        throw std::runtime_error(std::string("OpenCL error, code: ") + message);
    }
}

std::string EasyCL::getFileContents(std::string filename) {
    std::ifstream t(filename.c_str());
    std::stringstream buffer;
    buffer << t.rdbuf();
    return buffer.str();
}

//long EasyCL::getDeviceInfoInt(cl_device_info name) {
//    cl_ulong value = 0;
//    clGetDeviceInfo(device, name, sizeof(cl_ulong), &value, 0);
//    return static_cast<long>(value);
//}

int64_t EasyCL::getDeviceInfoInt64(cl_device_info name) {
    cl_ulong value = 0;
    clGetDeviceInfo(device, name, sizeof(cl_ulong), &value, 0);
    return static_cast<int64_t>(value);
}

void EasyCL::storeKernel(std::string name, CLKernel *kernel) {
    storeKernel(name, kernel, false);
}
// note that storing same name twice is an error, for now
// you can use name-mangling, or request I add a parameter 'bool overwrite'
// if deleteWithCl is true, then when this EasyCL object is deleted, this kernel
// will be deleted too
void EasyCL::storeKernel(std::string name, CLKernel *kernel, bool deleteWithCl) {
    if(kernelByName.count(name) != 0) {
        throw runtime_error("error: kernel for " + name + " already stored.");
    }
    kernelByName[ name ] = kernel;
    kernelOwnedByName[ name ] = deleteWithCl;
}
CLKernel *EasyCL::getKernel(std::string name) {
    return kernelByName[ name ];
}
bool EasyCL::kernelExists(std::string name) {
    return kernelByName.count(name) != 0;
}

