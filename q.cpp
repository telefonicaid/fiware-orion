#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <string>
#include <vector>



/* ****************************************************************************
*
* wsStrip - 
*/
static char* trim(char* s)
{
  // 1. beginning
  while ((*s == ' ') || (*s == '\t') || (*s == '\n'))
  {
    ++s;
  }

  if (*s == 0)
  {
    return s;
  }

  // 2. end
  char* end = &s[strlen(s) - 1];
  while ((end > s) && ((*end == ' ') || (*end == '\t') || (*end == '\n')))
  {
    *end = 0;
    --end;
  }

  return s;
}



/* ****************************************************************************
*
* qStringToBson - 
*/
int qStringToBson(char* in)
{
  char* str = strdup(in);
  char* s;
  int   statementNo = 0;

  while ((s = strtok(str, ";")) != NULL)
  {
    char*               left;
    char*               op;
    char*               right;
    char*               rangeFrom = (char*) "";
    char*               rangeTo   = (char*) "";
    std::vector<char*>  valVector;

    s = trim(s);

    printf("statement %d: %s\n", statementNo, s);

    left = s;
    if ((op = (char*) strstr(s, "==")) != NULL)
    {
      *op = 0;

      right = &op[2];
      op    = (char*) "==";

    }
    else if ((op = (char*) strstr(s, "!=")) != NULL)
    {
      *op = 0;

      right = &op[2];
      op    = (char*) "!=";
    }
    else if ((op = (char*) strstr(s, ">=")) != NULL)
    {
      *op = 0;

      right = &op[2];
      op    = (char*) ">=";
    }
    else if ((op = (char*) strstr(s, "<=")) != NULL)
    {
      *op = 0;

      right = &op[2];
      op    = (char*) "<=";
    }
    else if ((op = (char*) strstr(s, "<")) != NULL)
    {
      *op = 0;

      right = &op[1];
      op    = (char*) "<";
    }
    else if ((op = (char*) strstr(s, ">")) != NULL)
    {
      *op = 0;

      right = &op[1];
      op    = (char*) ">";
    }
    else if (s[0] == '-')
    {
      left  = (char*) "";
      op    = (char*) "NOT EXISTS";
      right = trim(&s[1]);
    }
    else if (s[0] == '+')
    {
      left  = (char*) "";
      op    = (char*) "EXISTS";
      right = trim(&s[1]);
    }
    else
    {
      printf("\nNo OP found\n");
      op    = (char*) "";
      right = (char*) "";
    }

    left  = trim(left);
    right = trim(right);
    

    if ((op == "==") || (op == "!="))
    {
      char* del;

      //
      // 1.  range?   A..B
      // 2.  list?    A,B,C
      // 2.1 if list, check single quotes
      //

      if ((del = strstr(right, "..")) != NULL)
      {
        *del = 0;
        rangeFrom = right;
        rangeTo   = &del[2];

        right     = (char*) "";
        rangeFrom = trim(rangeFrom);
        rangeTo   = trim(rangeTo);
      }
      else if ((del = strstr(right, ",")) != NULL)
      {
        char* start         = right;
        char* cP            = right;
        bool  insideQuotes  = false;
        bool  done          = false;

        while (done == false)
        {
          if (*cP == '\'')
          {
            insideQuotes = (insideQuotes == true)? false : true;
          }
          else if ((*cP == ',') || (*cP == 0))
          {
            // 1. If end-of-string reached, then this is the last loop
            if (*cP == 0)
            {
              done = true;
            }


            if (!insideQuotes)
            {
              //
              // If we are inside single-quotes, then do nothing, just advance the char pointer (++cP)
              // If not inside queotes, we are on a comma, so a new value is to be pushed onto the value vector.
              //

              // 2. Remove beginning quote, if there
              if (*start == '\'')
              {
                *start = 0;
                ++start;
              }

              // 3. Null-out the comma
              *cP = 0;

              // 4. Remove trailing quote, if there
              if (cP[-1] == '\'')
              {
                cP[-1] = 0;
              }

              // 5. Push the value onto the vector of values
              valVector.push_back(trim(start));

              // 6. Make point start to the beginning of the next value
              start = &cP[1];
            }
          }

          ++cP;
        }

        right = (char*) "";
      }
    }

    if (left      != "") printf("  left:        %s\n", left);
    if (op        != "") printf("  OP:          %s\n", op);
    if (right     != "") printf("  right:       %s\n", right);
    if (rangeFrom != "") printf("  rangeFrom:   %s\n", rangeFrom);
    if (rangeTo   != "") printf("  rangeTo:     %s\n", rangeTo);
    if (valVector.size() != 0)
    {
      for (unsigned int ix = 0; ix < valVector.size(); ++ix)
      {
        printf("  value %02d: \"%s\"\n", ix, valVector[ix]);
      }
    }
    printf("\n");

    ++statementNo;
    str = NULL;  // So that strtok continues eating the initial string
  }

  free(str);
  return 0;
}



/* ****************************************************************************
*
* main - 
*/
int main(int argC, char* argV[])
{
  char* s = (char*) "t == X , Y; t != X , Y; abc<5;acb>12;x==1,2,3;y!='4,2',5,6,'7,2';e>=5;f<=78,12;g==10..20;+h;-j";
  // char* s = (char*) "t=='1','2',3,4,' 5,6'";

  qStringToBson(s);
  return 0;
}
