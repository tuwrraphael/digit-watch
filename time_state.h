#ifndef APP_TIME_STATE
#define APP_TIME_STATE

#include "./cts_date.h"

typedef struct {
	cts_date_t cts_date;
	bool time_known;
} time_state_t;

#endif // !APP_TIME_STATE
