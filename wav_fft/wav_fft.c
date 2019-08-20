#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>
#include "wav.h"



int main(int argc, char* argv[])
{

    char* filename = argv[1];
    // Discovery platform
    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, NULL);
    // Discover device
    cl_device_id device;
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device,
    NULL);
    // Create context
    cl_context_properties props[3] = {CL_CONTEXT_PLATFORM, 
        (cl_context_properties)(platform), 0};
    cl_context context; 
    context = clCreateContext(props, 1, &device, NULL, NULL, 
    NULL);
    // Create command queue
    cl_command_queue queue;
    queue = clCreateCommandQueue(context, device, 0, NULL);

    WavFile *wf;

    wf = wav_open(filename,"r");
    
    uint16_t num_channel =  wav_get_num_channels(wf);
    uint32_t sample_rate = wav_get_sample_rate(wf);
    uint16_t bits_per_sample = wav_get_valid_bits_per_sample(wf);
    size_t   sample_size = wav_get_sample_size(wf);
    size_t   wav_length = wav_get_length(wf);
    uint32_t channel_mask = wav_get_channel_mask(wf);
    printf("filename : %s\nnum_channel : %u\nsample rate : %u\nbits per sample : %u \nsample size : %lu \nwav length = %lu \nchannel mask : %u\n",
    filename, num_channel,sample_rate,bits_per_sample,sample_size,wav_length,channel_mask);

    
    float *** buffers;
    uint32_t remain_count = ((wav_length*num_channel) >> 10);
    uint32_t count;
    buffers = (float ***)malloc(24*sizeof(float)*1024*num_channel);
    
    if(remain_count <24)
    {
        count = remain_count;
        remain_count = 0;
    }
    else
    {
        count =24;
        remain_count -= 24;
    }

    


    




}