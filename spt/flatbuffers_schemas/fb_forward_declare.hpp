#pragma once

#include "dbg.h"

/*
* Include this file in your headers to not drag in the whole flatbuffers library in every header.
* Also include it in your cpp files before including any generated flatbuffer schema files.
* 
* Make sure to include it *AFTER* SDK files.
*/

// SDK uses Verify macro for Asserts
#undef Verify

#define FLATBUFFERS_ASSERT Assert

// flatbuffer types
namespace flatbuffers
{
	template<bool>
	class FlatBufferBuilderImpl;

	using FlatBufferBuilder = FlatBufferBuilderImpl<false>;
} // namespace flatbuffers

// our types in the flatbuffer schemas
namespace fb
{
	/*
	* To not drag in the whole flatbuffer lib, return fb_off from Serialize() functions declared
	* in header files. This is implicitly convertable to flatbuffers::Offset<T>.
	* 
	* For example: `return builder.Finish().o`
	* Or: `fb:: ... ::Create...(fbb, ...).o`
	*/
	using fb_off = uint32_t;

	namespace math
	{
		struct Vector3;
		struct QAngle;
		struct Transform;
		struct Aabb;
		struct VPlane;
		struct Color32;
	} // namespace math

	namespace mesh
	{
		struct MbCompactMesh;
	}

} // namespace fb
