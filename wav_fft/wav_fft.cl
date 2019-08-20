__kernel
void fft_execute(__constant float *** inputdata,
                 __global float2 *** outputdata,
                 __local float2 ** localdata,
                 )
{
    int gid = get_group_id(0);
    int lid = get_local_id(0);
    int r_lid;
    for(int i=0;i<10;++i )
    {
        r_lid|=((lid>>i)&1)<<(9-i);
    }
    local[0][r_lid]=(inputdata[gid][0][lid],0.0f);
    
    barrier(CLK_LOCAL_MEM_FENCE);

    float PI = acos((float)-1);
    float2 w;
    for(int i=0,mask=1;i<10;i++,mask<<=1)
    {
        float theta = (2*PI>>i)*((mask-1)|lid);
        w.y=-sincos(theta,&w.x);
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

    }
    barrier[CLK_LOCAL_MEM_FENCE];

    outputdata[gid][lid]=localdata[lid];
}