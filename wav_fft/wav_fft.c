#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>
#include "wav.h"

char* readSource(char* kernelPath) {

   cl_int status;
   FILE *fp;
   char *source;
   long int size;

   printf("Program file is: %s\n", kernelPath);

   fp = fopen(kernelPath, "rb");
   if(!fp) {
      printf("Could not open kernel file\n");
      exit(-1);
   }
   status = fseek(fp, 0, SEEK_END);
   if(status != 0) {
      printf("Error seeking to end of file\n");
      exit(-1);
   }
   size = ftell(fp);
   if(size < 0) {
      printf("Error getting file position\n");
      exit(-1);
   }

   rewind(fp);

   source = (char *)malloc(size + 1);

   int i;
   for (i = 0; i < size+1; i++) {
      source[i]='\0';
   }

   if(source == NULL) {
      printf("Error allocating space for the kernel source\n");
      exit(-1);
   }

   fread(source, 1, size, fp);
   source[size] = '\0';

   return source;
}

void chk(cl_int status, const char* cmd) {

   if(status != CL_SUCCESS) {
      printf("%s failed (%d)\n", cmd, status);
      exit(-1);
   }
}
int main(int argc, char* argv[])
{

    char* filename = argv[1];
    cl_int status;

    // Discovery platform
    cl_platform_id platform;
    status=clGetPlatformIDs(1, &platform, NULL);
    chk(status, "clGetPlatformIDs");

    // Discover device
    cl_device_id device;
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device,
    NULL);
    // Create context
    cl_context_properties props[3] = {CL_CONTEXT_PLATFORM, 
        (cl_context_properties)(platform), 0};
    cl_context context; 
    context = clCreateContext(props, 1, &device, NULL, NULL, 
    &status);
   chk(status, "clCreateContext");
    // Create command queue
    cl_command_queue queue;
    queue = clCreateCommandQueue(context, device, 0,&status);
   chk(status, "clCreateCommandQueue");
    const char * source = readSource("wav_fft.cl");
    cl_program program = clCreateProgramWithSource(context, 1, &source, NULL,&status);   
   chk(status, "clCreateProgramWithSource");
    status = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    chk(status, "clBuildProgram");
    cl_kernel kernel;
    kernel = clCreateKernel(program, "fft", &status);
    chk(status,"clCreateKernel");
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

    cl_ushort* buffers;
    cl_float2 * obuffers;
    buffers = malloc(num_channel*24*1024*sample_size);
    obuffers = malloc(num_channel*24*1024*sizeof(cl_float2));
    for(int i=0;i<num_channel*24*1024;i++)
    {
        obuffers[i].x=0.0f;
        obuffers[i].y=0.0f;
    }
    uint32_t remain_count = (wav_length >> 10);
    uint32_t count;
    cl_mem buf_1 = clCreateBuffer(context, CL_MEM_READ_ONLY, 
       24*1024*sizeof(cl_ushort),NULL, &status);
   chk(status, "clCreateBuffer");
    cl_mem buf_2 = clCreateBuffer(context, CL_MEM_READ_ONLY, 
       24*1024*sizeof(cl_ushort),NULL, &status);
   chk(status, "clCreateBuffer");
    cl_mem output_buf = clCreateBuffer(context,CL_MEM_WRITE_ONLY,
       24*1024*sizeof(cl_float2),NULL,&status);
   chk(status, "clCreateBuffer");
   int j=0;
    while(1)
    {
        if (remain_count<1)
        {
            break;
        }
        else if(remain_count <24)
        {
            count = remain_count;
            remain_count = 0;
        }
        else
        {
            count =24;
            remain_count -= 24;
        }
        wav_read(wf, &buffers,1024*count); 
        status = clEnqueueWriteBuffer(queue, buf_1, CL_FALSE, 0, 
        count*1024*sizeof(cl_ushort), &buffers[0], 0, NULL, NULL);
        chk(status, "clEnqueueWritebuffer");
        // Create the kernel object
        status = clSetKernelArg(kernel,0,sizeof(cl_mem),&buf_1);
        chk(status, "clSetKernelArg1");
        status = clSetKernelArg(kernel,1,sizeof(cl_mem),&output_buf);
        chk(status, "clSetKernelArg2");
        status = clSetKernelArg(kernel,2,1024*num_channel*(sizeof(cl_float2)),NULL);
        chk(status, "clSetKernelArg3");
        size_t global_work_size[1];
        global_work_size[0] = 1024*count;
        size_t local_work_size[1];
        local_work_size[0]=1024;
        printf("\nkerenl execute count : %d\n",count);
        status = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, global_work_size,local_work_size, 0, NULL, NULL);
        chk(status, "clEnqueueNDRangeKernel");
        clFinish(queue);
        printf("kerenl complete %d \n",++j);
        status = clEnqueueReadBuffer(queue,output_buf,CL_FALSE,0,count*1024*sizeof(cl_float2),&obuffers[0],0,NULL,NULL);
        chk(status,"clEnqueueReadBuffer");
    }
    clReleaseMemObject(buf_1);
    clReleaseMemObject(buf_2);
    clReleaseMemObject(output_buf);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    clReleaseDevice(device);
    
    wav_close(wf);
    
    free(buffers);
    free(obuffers);


    return;
}