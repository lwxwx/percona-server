#ifndef BENCHMARK_HANDLER_HEADER
#define BENCHMARK_HANDLER_HEADER

#include <cstdint>

namespace BenchLog {

enum ConflictLevel
{
	PAGE = 1,
	ROW =  2
};

struct BenchLogRecord
{
	uint32_t page_no;
	uint32_t row_no;
};
//
// bool is_conflict(const BenchLogRecord & ref,const BenchLogRecord & cur,ConflictLevel level = ConflictLevel::ROW)
// {
//   if(ref.page_no == cur.page_no)
//   {
//     return true;
//   }
//   if(level != ROW)
//   {
//     return false;
//   }
//   if(ref.row_no == cur.row_no)
//   {
//     return true;
//   }
// }
}


#endif
