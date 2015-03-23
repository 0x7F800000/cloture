#pragma once

namespace cloture::util
{
	class meta : __markAsCtfe(), __markApiObject()
	{
	private:
		struct metaBool	: __markAsCtfe()
		{} __unused;

		template<typename T>
		static constexpr bool internalMetaBoolValue()
		{
			static_assert(
			isMetaBool<T>,
			"metaBoolValue was provided a non-metaBool type.");
			return _Generic((*static_cast<T*>(nullptr)),
				const metaFalse:
					false,
				const metaTrue:
					true,
				volatile metaFalse:
					false,
				volatile metaTrue:
					true,
				metaFalse:
					false,
				metaTrue:
					true
				);
		}

		template<typename T>
		static constexpr int internalMetaIntValue()
		{
			static_assert(
			isMetaInt<T>,
			"isMetaInt was provided a non-metaInt type."
			);

			static_assert(
					sizeof(T::_internal_value_::value[0]) == 1,
					"Recheck the definition of metaInt. Something was changed."
			);
			return static_cast<int>(sizeof(T::_internal_value_::value));
		}

		struct _metaInt : __markAsCtfe()
		{} __unused;

	public:
		template<int metaval>
		struct metaInt : __markAsCtfe(), public _metaInt
		{
			struct _internal_value_
			{
				__unused bool value[metaval];
			}__unused;
		}__unused;

		struct metaTrue : public metaBool, __markAsCtfe()
		{
			using isTrue = void;
			static constexpr auto clotureTypeName()
			{
				return "metaBool<value = True>";
			}
		}__unused;

		struct metaFalse : public metaBool, __markAsCtfe()
		{
			using isFalse = void;
			static constexpr auto clotureTypeName()
			{
				return "metaBool<value = False>";
			}
		}__unused;

		struct metaNull : __markAsCtfe()
		{
			static constexpr auto clotureTypeName()
			{
				return "(NULL type)";
			}
		}__unused;

		template<bool value>
		struct makeMetabool
		{

		}__unused;

		template<>
		struct makeMetabool<false>
		{
			using type = metaFalse;
		}__unused;

		template<>
		struct makeMetabool<true>
		{
			using type = metaTrue;
		}__unused;

		template<typename T>
		static constexpr bool isMetaNull = false;

		template<>
		static constexpr bool isMetaNull<metaNull> = true;

		template<typename T>
		static constexpr bool isMetaBool = false;

		template<>
		static constexpr bool isMetaBool<metaTrue> = true;

		template<>
		static constexpr bool isMetaBool<metaFalse> = true;

		template<typename T>
		static constexpr bool isMetaInt = __is_base_of(_metaInt, T);

		/*
		 * remember to check with isMetaBool before using this
		 */
		template<typename T>
		static constexpr bool metaBoolValue = internalMetaBoolValue<T>();

		template<typename T>//const char* className>
		struct namedClass
		{
			static constexpr auto clotureTypename()
			{
				return T::str;//className;
			}
		};

	#define		__clClassname(name)		\
	public cloture::util::meta::namedClass<mMetaString("nameTest", 128)>

	};
}

#define		mIfMetaTrue(condition)		__if_exists(cloture::util::meta::makeMetabool<condition>::type::isTrue)
#define		mIfMetaFalse(condition)		__if_not_exists(cloture::util::meta::makeMetabool<condition>::type::isTrue)

