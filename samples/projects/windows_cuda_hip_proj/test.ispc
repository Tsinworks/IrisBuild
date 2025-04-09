#include "test.h"

#include "Aossoa.isph"
#include "Soaaos.isph"

inline varying FVector VectorLoad(const uniform FVector *uniform SrcPtr)
{
	varying FVector Result;

	if(((1<<TARGET_WIDTH)-1 ^ lanemask()) == 0)
	{
		aos_to_soa3_ispc((uniform float* uniform)SrcPtr, 
			(varying float* uniform)&Result.X, 
			(varying float* uniform)&Result.Y, 
			(varying float* uniform)&Result.Z);
	}
	else
	{
		#pragma ignore warning(perf)
		Result = SrcPtr[programIndex];
	}

	return Result;
}

inline void VectorStore(uniform FVector *uniform DstPtr, const varying FVector &V)
{
	if(((1<<TARGET_WIDTH)-1 ^ lanemask()) == 0)
	{
		soa_to_aos3_ispc(V.X, V.Y, V.Z, (uniform float *uniform)DstPtr);
	}
	else
	{
		#pragma ignore warning(perf)
		DstPtr[programIndex] = V;
	}
}

export void Add(const uniform FVector InA[], const uniform FVector InB[], uniform FVector OutC[], uniform int Num)
{
	foreach (i = 0...Num)
	{
		const uniform int Index = extract(i, 0);
		FVector A = VectorLoad(&InA[Index]);
		FVector B = VectorLoad(&InB[Index]);
		VectorStore(&OutC[Index], A+B);
	}
}