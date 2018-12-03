#include <time.h>                                              // time, gmtime_r



// -----------------------------------------------------------------------------
//
// numberToDate -
//
bool numberToDate(time_t fromEpoch, char* date, int dateLen, char** detailsP)
{
  struct tm  tm;

  gmtime_r(&fromEpoch, &tm);
  strftime(date, dateLen, "%Y-%m-%dT%H:%M:%S", &tm);

  return true;
}
