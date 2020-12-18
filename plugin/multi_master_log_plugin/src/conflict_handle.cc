#include "conflict_handle.h"
#include <bits/stdint-uintn.h>
#include <cstdlib>
#include "mmlp_type.h"
#include "trx_log.h"
#include <cstdlib>

void ConflictHandle::init(ConflictHandleLevel c_level, ConflictHandleType h_type)
{
	handle_level = c_level;
	handle_type = h_type;
}

bool ConflictHandle::arg_detect(TrxID id)
{
	// double page_percent = conflict_page_percent * 0.01;
	// double row_percent = conflict_row_percent * 0.01;
  bool page_is = false;
	bool row_is = false;
	for(int i = 0; i < conflict_trx_length ; i++)
	{
		srand(conflict_trx_length * 7 - 1 + id);
		
		page_is = rand() % 101 < conflict_page_percent;
		row_is = rand() % 101 < conflict_row_percent;
		if(conflict_detect_level == ConflictHandleLevel::PAGE)
		{
			if(page_is)
			{
				return true;
			}
		}
		if(conflict_detect_level == ConflictHandleLevel::ROW)
		{
			if(page_is && row_is)
			{
				return true;
			}
		}
	}
}

// int ConflictHandle::conflict_check(TrxLog *trx)
// {
//     switch (handle_type) {
//         case ConflictHandleType::PERCENTAGE_CONFLICT_HANDLE:
//             return percent_conflict_check(trx);
//             break;
//     }
// }
//
/**
 * multi type conflict handle functions
 * **/
// int ConflictHandle::percent_conflict_check(TrxLog * trx)
// {
//     srand(trx->get_trxID());
//     auto res_val = rand()%100;
//
//     if(res_val < *(int *)arg_map[MAX_PERCENT])
//     {
//         return 1;
//     }
//     else
//     {
//         return -1;
//     }
// }
//
