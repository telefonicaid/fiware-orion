#include <stdio.h>
#include <time.h>
#include <string.h>
#include <string>


static struct timespec t1;
static struct timespec t2;
static struct timespec tDiffCpp;
static struct timespec tDiffC;



/* ****************************************************************************
*
* cpp_string_compare - 
*/
const std::string& cpp_string_compare(const std::string& s1, const std::string& s2)
{
  if (s1 > s2)
    return s1;
  return s2;
}



/* ****************************************************************************
*
* c_string_compare - 
*/
const char* c_string_compare(const char* s1, const char* s2)
{
  if (strcmp(s1, s2) > 0)
    return s1;
  return s2;
}



/* ****************************************************************************
*
* clock_difftime - 
*/
void clock_difftime(const struct timespec* endTime, const struct timespec* startTime, struct timespec* diffTime)
{
  diffTime->tv_nsec = endTime->tv_nsec - startTime->tv_nsec;
  diffTime->tv_sec  = endTime->tv_sec  - startTime->tv_sec;

  if (diffTime->tv_nsec < 0)
  {
    diffTime->tv_sec -= 1;
    diffTime->tv_nsec += 1000000000;
  }
}



//#define S1 "1111111111122222222222233333333333333344444444444444455555555555555556666666666666667777777777777777778888888888999999999999999999"
//#define S2 "2111111111122222222222233333333333333344444444444444455555555555555556666666666666667777777777777777778888888888999999999999999999"

#define S1 "2"
#define S2 "1"

//#define S1 ""
//#define S2 ""


/* ****************************************************************************
*
* main - 
*/
int main(int argC, char* argV[])
{
  clock_gettime(CLOCK_REALTIME, &t1);
  std::string s = cpp_string_compare(S1, S2);
  clock_gettime(CLOCK_REALTIME, &t2);
  clock_difftime(&t2, &t1, &tDiffCpp);

  clock_gettime(CLOCK_REALTIME, &t1);
  const char* cp = c_string_compare(S1, S2);
  clock_gettime(CLOCK_REALTIME, &t2);
  clock_difftime(&t2, &t1, &tDiffC);

  printf("C++: %d.%09d\n", tDiffCpp.tv_sec, tDiffCpp.tv_nsec);
  printf("C:   %d.%09d\n", tDiffC.tv_sec,   tDiffC.tv_nsec);

  int cpp   = tDiffCpp.tv_nsec;
  int c     = tDiffC.tv_nsec;

  printf("C++: %d\n", cpp);
  printf("C:   %d\n", c);

  float ratio = (float) cpp / (float) c;

  printf("char* is %f times faster than std::string\n", ratio);

  return 0;
}
