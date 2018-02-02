#ifndef APP_CTS_DATE
#define APP_CTS_DATE

#include <stdint.h>

typedef struct {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t dayOfWeek;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	uint8_t fraction;
} cts_date_t;

#endif // !APP_CTS_DATE
