
#include "util/common.hpp"
using namespace cloture::util::common;
extern "C"
{
	
int __llvm_profile_runtime;
struct __llvm_profile_data
{
	unsigned int NameSize;
	unsigned int NumCounters;
	uint64 FuncHash;
	const char* Name;
	uint64 * Counters;
};
static const __llvm_profile_data *DataFirst = nullptr;
static const __llvm_profile_data *DataLast = nullptr;
static const char *NamesFirst = nullptr;
static const char *NamesLast = nullptr;
static uint64 *CountersFirst = nullptr;
static uint64 *CountersLast = nullptr;

/*!
 * \brief Register an instrumented function.
 *
 * Calls to this are emitted by clang with -fprofile-instr-generate.  Such
 * calls are only required (and only emitted) on targets where we haven't
 * implemented linker magic to find the bounds of the sections.
 */

void __llvm_profile_register_function(void *Data_) 
{
  /* TODO: Only emit this function if we can't use linker magic. */
  const __llvm_profile_data *Data = (__llvm_profile_data*)Data_;
  if (!DataFirst) 
  {
    DataFirst = Data;
    DataLast = Data + 1;
    NamesFirst = Data->Name;
    NamesLast = Data->Name + Data->NameSize;
    CountersFirst = Data->Counters;
    CountersLast = Data->Counters + Data->NumCounters;
    return;
  }

#define UPDATE_FIRST(First, New) \
  First = New < First ? New : First
  UPDATE_FIRST(DataFirst, Data);
  UPDATE_FIRST(NamesFirst, Data->Name);
  UPDATE_FIRST(CountersFirst, Data->Counters);
#undef UPDATE_FIRST

#define UPDATE_LAST(Last, New) \
  Last = New > Last ? New : Last
  UPDATE_LAST(DataLast, Data + 1);
  UPDATE_LAST(NamesLast, Data->Name + Data->NameSize);
  UPDATE_LAST(CountersLast, Data->Counters + Data->NumCounters);
#undef UPDATE_LAST
}

const __llvm_profile_data *__llvm_profile_data_begin(void) 
{
  return DataFirst;
}
const __llvm_profile_data *__llvm_profile_data_end(void) 
{
  return DataLast;
}
const char *__llvm_profile_names_begin(void) { return NamesFirst; }
const char *__llvm_profile_names_end(void) { return NamesLast; }
uint64 *__llvm_profile_counters_begin(void) { return CountersFirst; }
uint64 *__llvm_profile_counters_end(void) { return CountersLast; }


};
