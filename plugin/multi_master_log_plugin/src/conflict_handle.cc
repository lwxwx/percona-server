#include "conflict_handle.h"
#include <cstdlib>
#include "trx_log.h"
#include "stdlib.h"

int ConflictHandle::conflict_check(TrxLog *trx)
{
	switch (handle_type) {
		case ConflictHandleType::PERCENTAGE_CONFLICT_HANDLE:
			return percent_conflict_check(trx);
			break;
	}
}

/**
 * multi type conflict handle functions
 * **/
int ConflictHandle::percent_conflict_check(TrxLog * trx)
{
	srand(trx->get_trxID());
	auto res_val = rand()%100;

	if(res_val < *(int *)arg_map[MAX_PERCENT])
	{
		return 1;
	}
	else
	{
		return -1;
	}
}

