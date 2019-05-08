#define EPSILON 0.01f
#define MAX_ITERATIONS 5000
#define ESCAPE_RADIUS 64.f

unsigned int rand( unsigned int *seed ) // 1 <= *seed < 2147483647
{
    *seed = ( (unsigned long)( *seed )* 16807UL ) % 2147483647UL;
    return *seed;
}

float halton(int index, int base)
{
    float result = 0.f;
    float f = 1 / (float)base;
    int i = index;
    while(i > 0){
        result += f * (i % base);
        i = (int)(i / base);
        f /= (float)base;
    }

    return result;
}

float randf( unsigned int *seed )
{
	unsigned int r = rand( seed );
	return ( (float)( r - 1 ) / (float)( 2147483647 - 2 ) );
}

__kernel void buddhabrot(
	volatile __global uint *image,
	//__global unsigned int *seedArray,
	int width,
	int height,
	int samples,
	float rStart,
	float rExtent,
	float iStart,
	float iExtent)
{
	const int2 pos = { get_global_id(0), get_global_id(1) };
	unsigned int index = pos.x + width * pos.y;
	
	// Get the seed.
	//unsigned int seed = seedArray[index];

	const float rSampleStart = -2.2f;
	const float rSampleExtent = 3.f;

	//float iExtent = rSampleExtent / height * width;
	float iSampleExtent = rSampleExtent / rExtent * iExtent;
	float iSampleStart = -iSampleExtent / 2;

	//float cr = rStart + rExtent / height * ((float)pos.y + randf(&seed) - 0.5f);
	//float ci = iStart + iExtent / width * ((float)pos.x + randf(&seed) - 0.5f);
    float cr = rSampleStart + rSampleExtent / height * ((float)pos.y + halton(samples + 1, 2) - 0.5f);
	float ci = iSampleStart + iSampleExtent / width * ((float)pos.x + halton(samples + 1, 3) - 0.5f);

	float zr = 0;
	float zi = 0;
	int i;
	for(i = 0; i < MAX_ITERATIONS; ++i){
	    float tmp = zr;
	    zr = zr*zr - zi*zi + cr;
	    zi = 2*tmp*zi + ci;

	    if(zr*zr+zi*zi > ESCAPE_RADIUS){
	        break;
	    }
	}

	if(i != MAX_ITERATIONS){
		float zr = 0;
    	float zi = 0;
    	for(int j = 0; j < MAX_ITERATIONS; ++j){
    	    float tmp = zr;
    	    zr = zr*zr - zi*zi + cr;
    	    zi = 2*tmp*zi + ci;
    	    
            if(zr*zr + zi*zi > ESCAPE_RADIUS){
                break;
            }

            int x = (-fabs(zi) - iStart) / iExtent * width;
            //int x = (zi - iStart) / iExtent * width;
            if (x >= 0 && x < width && fabs(zi) < fabs(iStart)) {
                int y = (zr - rStart) / rExtent * height;
                if (y >= 0 && y < height && zr > rStart) {
                    int offset = 3*(x + y * width);
                    if (i < 50) {
                        offset += 2;
                    }
                    else if (i < 500) {
                        offset += 1;
                    }
                    // This is by far the slowest part, which is why we do all the strange stuff above.
                    // (mirroring, only writing to one channel and then add them together again on the cpu...)
                    atomic_inc(&image[offset]);
                }
            }
    	}
	}
}