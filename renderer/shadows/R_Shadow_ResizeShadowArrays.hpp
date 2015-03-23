#pragma once

template<typename T, size32 elementCount>
static inline void resizeShadowArrayElements(
	const int numElements,
	const int elementScale,
	int& maxElements,
	T*& elements
)
{
	const int scaledElements = ((numElements + 255) & ~255) * elementScale;

	if(maxElements >= scaledElements)
		return;

	maxElements = scaledElements;

	if( !isNull(elements) )
		r_main_mempool->dealloc(elements);

	elements = r_main_mempool->alloc<T[elementCount], true>(scaledElements);


}

static void R_Shadow_ResizeShadowArrays(int numvertices, int numtriangles, int vertscale, int triscale)
{
	resizeShadowArrayElements<int, 3>(		numtriangles, 	triscale, 	maxshadowtriangles, 	shadowelements);
	resizeShadowArrayElements<float, 3>(	numvertices, 	vertscale, 	maxshadowvertices, 		shadowvertex3f);
}
