#pragma once

#define		mVec256Align	32

template<typename T> struct pickVecType{using type = void;};

#define		pickVecTypeSpecial(typeName)	\
		template<>struct pickVecType<common::typeName>	\
		{\
			typedef common::typeName type __attribute__((__vector_size__(16)));\
		}

	pickVecTypeSpecial(int8);
	pickVecTypeSpecial(uint8);

	pickVecTypeSpecial(int16);
	pickVecTypeSpecial(uint16);

	pickVecTypeSpecial(int32);
	pickVecTypeSpecial(uint32);

	pickVecTypeSpecial(int64);
	pickVecTypeSpecial(uint64);

	pickVecTypeSpecial(real32);
	pickVecTypeSpecial(real64);

#undef pickVecTypeSpecial

template<typename elementType, typename inputType, typename... others>
class initVecHelper
{
	template<typename eleType, size_t paramcount, typename inputType_, typename... others_>
	struct init
	{

		//mVecInline __pure static void doInit
	};
	template<typename eleType, typename inputType_>
	struct init<
		eleType,
		2,
		inputType_,
		typename pickVecType<eleType>::type,
		typename pickVecType<eleType>::type
	>
	{
		using paramType = typename pickVecType<eleType>::type;
		mVecCall __pure static void doInit(inputType out1, inputType out2, paramType in1, paramType in2)
		{
			out1 = in1, out2 = in2;
		}
	};
public:
	using initializer = init<elementType, sizeof...(others), inputType, others...>;
};

template<typename T, typename... others>
mVecCall __pure void initVec(
		typename pickVecType<T>::type& out1,
		typename pickVecType<T>::type& out2,
		others... Others
)
{
	using initializer = typename initVecHelper<T, __typeof__(out1), others...>::initializer;
	initializer::doInit(out1, out2, Others...);
}

template<typename T>
__align(mVec256Align) struct vector256
{

	union
	{
		struct
		{
			__m128 vec1;
			__m128 vec2;
		};
		vec_t arr[8];
	};
	mMarkAggregateVec();
	using vecType = vector256<T>;
	using constele = const __typeof(vec1[0]);

	constexpr mVecPure vector256(
	constele v0, constele v1, constele v2, constele v3,
	constele v4, constele v5, constele v6, constele v7
	)
	{
		vec1 = (__typeof(vec1)){v0, v1, v2, v3};
		vec2 = (__typeof(vec2)){v4, v5, v6, v7};
	}

	constexpr mVecPure vector256(const __typeof(vec1) v0, const __typeof(vec2) v1)
	{
		vec1 = v0;
		vec2 = v1;
	}

	constexpr mVecPure vector256(constele v0)
	{
		vec2 = vec1 = (__typeof(vec1)){v0, v0, v0, v0};
	}

	constexpr mVecPure vector256(const __typeof(vec1[0])* RESTRICT const vIn)
	{
		vec1 = (__typeof(vec1)){vIn[0], vIn[1], vIn[2], vIn[3]};
		vec2 = (__typeof(vec1)){vIn[4], vIn[5], vIn[6], vIn[7]};
	}
};

static_assert(isAggregateVector<
vector256<float>
 >);


