__kernel
void fft(__constant ushort * inputdata,
                 __global float2 * outputdata,
                 __local float2 * localdata)
{
    int gid = get_group_id(0);
    int ls = get_local_size(0);
    int lid = get_local_id(0);
    int r_lid=0;
    for(int i=0;i<10;++i )
    {
        r_lid|=((lid>>i)&1)<<(9-i);
    }
    localdata[r_lid].x=(float)inputdata[gid*ls+lid];
    barrier(CLK_LOCAL_MEM_FENCE);
    //printf("%d = %u ->%f\n",lid,inputdata[gid*ls+lid],localdata[lid].x);

    float PI = acos(-1.0f);
    float2 w;
    int mask =1;
    for(int i=0;i<10;i++)
    {
        float theta = PI*(2>>i)*((mask-1)&lid);
        w.x=cos(theta);
        w.y=-sin(theta);
        float2 tmp1 = localdata[lid&(~mask)];
        float2 tmp2 = localdata[lid|mask];
        float2 tmp3;
        tmp3.x = tmp2.x*w.x-tmp2.y*w.y;
        tmp3.y = tmp2.x*w.y+tmp2.y*w.x;
        barrier(CLK_LOCAL_MEM_FENCE);
        if(mask&lid)
            {
                localdata[lid]=tmp1+tmp3;
            }
        else
            {
                localdata[lid]=tmp1-tmp3;
            }
        mask<<=1;
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    outputdata[gid*ls+lid]=localdata[lid];
}