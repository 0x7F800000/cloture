#pragma once

namespace cloture {
namespace util    {
namespace common  {
  /*
  integer primitive types
  */
  using uint8 = unsigned char;
  using int8 = signed char;
  using uint16 = unsigned short;
  using int16 = signed short;
  using uint32 = unsigned int;
  using int32 = signed int;
  using uint64 = unsigned __int64;//unsigned long long;
  using int64 = __int64;//signed long long;
  /*
  sanity checks
  */
  static_assert( sizeof(uint8) == 1	&& sizeof(int8) == 1, "sizeof int8/uint8 ought to be one byte...");
  static_assert( sizeof(uint16) == 2	&& sizeof(int16) == 2, "sizeof int16/uint16 should be two bytes...");
  static_assert( sizeof(uint32) == 4	&& sizeof(int32) == 4, "sizeof int32/uint32 should be four bytes...");
  static_assert( sizeof(uint64) == 8	&& sizeof(int64) == 8, "sizeof int64/uint64 should be eight bytes...");
  /*
  real number types
  */
  using real32 = float;
  using real64 = double;
  using real128 = long double;

}//namespace common
}//namespace util
}//namespace cloture
