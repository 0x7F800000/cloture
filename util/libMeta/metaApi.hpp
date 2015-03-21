#pragma once

namespace cloture::util
{
	class meta : __markAsCtfe(), __markApiObject()
	{
	private:
		struct metaBool	: __markAsCtfe()
		{};

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
		{};

	public:
		template<int metaval>
		struct metaInt : __markAsCtfe(), public _metaInt
		{
			struct _internal_value_
			{
				bool value[metaval];
			};
		};

		struct metaTrue : public metaBool, __markAsCtfe()
		{
			static constexpr auto clotureTypeName()
			{
				return "metaBool<value = True";
			}
		};

		struct metaFalse : public metaBool, __markAsCtfe()
		{
			static constexpr auto clotureTypeName()
			{
				return "metaBool<value = False";
			}
		};

		struct metaNull : __markAsCtfe()
		{
			static constexpr auto clotureTypeName()
			{
				return "(NULL type)";
			}
		};

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
