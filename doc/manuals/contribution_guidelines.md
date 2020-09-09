# Orion Context Broker Contribution Guide

This document describes the guidelines to contribute to Orion Context Broker. If you are
planning to contribute to the code you should read this document and get familiar with its content.

## General principles

* Orion Context Broker programming languages are C and C++ (although some side pieces related with script tooling
  are written in bash and Python).
* Efficient code (i.e. the one that achieves better performance) is preferred upon inefficient code. Simple code
  (i.e. cleaner and shorter) is preferred upon complex code. Big savings in efficiency with small penalty in
  simplicity are allowed. Big savings in simplicity with small penalty in efficiency are also allowed.
* New files contributed to Orion must follow the [filesystem layout guidelines](#filesystem-layout-guidelines).
* Code contributed to Orion Context Broker must follow the [code style guidelines](#code-style-guidelines) 
  in order to set a common programming style for all developers working on the code.
* See also the section on [programming patterns](#programming-patterns) about which ones are allowed and disallowed in the 
  Orion Context Broker code.

Note that contribution workflows themselves (e.g. pull requests, etc.) are described in another document 
([FIWARE Development Guidelines](https://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/Developer_Guidelines)).

## Pull Request protocol

As explained in ([FIWARE Development Guidelines](https://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/Developer_Guidelines)) 
contributions are done using a pull request (PR). The detailed "protocol" used in such PR is described below:

* Direct commits to master branch (even single-line modifications) are not allowed. Every modification has to come as a PR
* In case the PR is implementing/fixing a numbered issue, the issue number has to be referenced in the body of the PR at creation time
* Anybody is welcome to provide comments to the PR (either direct comments or using the review feature offered by Github)
* Use *code line comments* instead of *general comments*, for traceability reasons (see comments lifecycle below)
* Comments lifecycle
  * Comment is created, initiating a *comment thread*
  * New comments can be added as responses to the original one, starting a discussion
  * After discussion, the comment thread ends in one of the following ways:
    * `Fixed in <commit hash>` in case the discussion involves a fix in the PR branch (which commit hash is 
       included as reference)
    * `NTC`, if finally nothing needs to be done (NTC = Nothing To Change)
 * PR can be merged when the following conditions are met:
    * All comment threads are closed
    * All the participants in the discussion have provided a `LGTM` general comment (LGTM = Looks good to me)
 * Self-merging is not allowed (except in rare and justified circumstances)

Some additional remarks to take into account when contributing with new PRs:

* PR must include not only code contributions, but their corresponding pieces of documentation (new or modifications to existing one) and tests
* PR modifications must pass full regression based on existing test (unit, functional, memory, e2e) in addition to whichever new test added due to the new functionality
* PR should be of an appropriated size that makes review achievable. Too large PRs could be closed with a "please, redo the work in smaller pieces" without any further discussing

## Filesystem layout guidelines

### Directory layout

For a detailed explanation of the directory structure, see [this section in the Development Manual](devel/directoryStructure.md).

### File layout for source code files 

Source code files are found under the `src/` directory.

The suffix '.cpp' MUST be used for source files and '.h' MUST be used for header files.

As a general rule, for C/C++ source code, every concept SHOULD have its own module. With a module is referred a 
header file (`.h`) and its corresponding source file (`.cpp`). In some cases only a header file is needed (without any corresponding `.cpp` 
source file): header files containing only constants, macros or inline functions.

A class SHOULD reside in its own module, with the class definition in the header file
and the implementation of the classes in the source file. The name for this module is the name of the class. E.g. 
the class Subscription resides in the module { `Subscription.h` / `Subscription.cpp` }.

If a class requires related classes (e.g. subclasses or embedded classes), they should reside in the same module, 
but only if these classes are not used anywhere else. If used anywhere else, a new module MUST be created for the 
affected classes.

## Code style guidelines

Note that currently not all Orion existing code base conforms to these rules. There are some parts of the code that were 
written before the guidelines were established. However, all new code contributions MUST follow these rules and, eventually, old code will be modified to conform to the guidelines.

### ‘MUST follow’ rules 

#### M1 (Headers Files Inclusion):

*Rule*: All header or source files MUST include all the header files it needs AND NO OTHER header files. They MUST
NOT depend on inclusions of other header files. Also, all header and source files MUST NOT include any header files it 
does not need itself.

*Rationale*: each file should not depend on the inclusions other files have/doesn’t have. Also, if a header file
includes more files than it needs, its ‘clients’ has no other choice than to include those ‘extra’ files as
well. This sometimes leads to conflicts and must be avoided. In addition, it increases the compilation time.

*How to check*: manually

#### M2 (Copyright header)

*Rule*: Every file, source code or not, MUST have a copyright header:

For C++ files:

```
/*
*
* Copyright 20XX Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: <the author>
*/
```

For Python, bash script, CMakeLists.txt, etc.:

```
# Copyright 20XX Telefonica Investigacion y Desarrollo, S.A.U
#
# This file is part of Orion Context Broker.
#
# Orion Context Broker is free software: you can redistribute it and/or
# modify it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# Orion Context Broker is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
# General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
#
# For those usages not covered by this license please contact with
# iot_support at tid dot es
#
# Author: <the author>
```

*Rationale*: to have an homogenous copyright header for all files.

*How to check*": checked using either the internal script ```scripts/check_files_compliance.py```, or ```scripts/style_check.sh```

#### M3 (Function header)

*Rule*: All functions MUST have a header, which SHOULD have a short description of what the function does, a 
descriptive list of its parameters, and its return values.

Example:

```
/* ****************************************************************************
*
* parseUrl - parse a URL and return its pieces
*
*  [ Short description if necessary ]
*
* PARAMETERS
*   - url         The URL to be examined
*   - host        To output the HOST of the URL 
*   - port        To output the PORT of the URL
*   - path        To output the PATH of the URL
*   - protocol    To output the PROTOCOL of the URL
*
* RETURN VALUE
*   parseUrl returns TRUE on successful operation, FALSE otherwise
*
* NOTE
*   About the components in a URL: according to 
*   https://tools.ietf.org/html/rfc3986#section-3,
*   the scheme component is mandatory, i.e. the 'http://' or 'https://' must 
*   be present, otherwise the URL is invalid.
*/
```

*Rationale*: the code is simply easier to read when prepared like this

*How to check*": manually

#### M4 (Indent)

*Rule*: Use only spaces (i.e. no tabs), and indent TWO spaces at a time.
 
*Rationale*: two whitespaces is enough. It makes the lines not too long

*How to check*": checked using the internal script ```scripts/style_check.sh```

#### M5 (Variable declaration):

*Rule*: Each declared variable MUST go on a separate line:

```
int a;
int b;
```

The following usage MUST be avoided:

```
int a, b;
```

*Rationale*: easier to read.

*How to check*: manually

#### M6 (Naming conventions):

*Rule*: the following naming conventions apply:

* CamelCase for class/struct names, e.g. `SubscriptionResponse`.
* UPPER_LETTER for macro names and `#define` constant, e.g. `LM_E(...)`, `EARTH_RADIUS`
* camelCase for all other cases, e.g. `connectionMemory`.

*Rationale*: this rule makes it easy to understand whether we’re dealing with a macro or a constant, or a variable  
for that matter. Same with function vs macro.

*How to check*: manually

#### M7 (Multi line macros):

*Rule*: Macros spanning over multiple lines MUST have the backslashes horizontally aligned:

```
#define LONG_MACRO(a, b)    \
do                          \
{                           \
  action();                 \
} while 0;
```

*Rationale*: easier on the eye.

*How to check*: manually

#### M8 (Code blocks):

*Rule*:The open and close brackets `{` and `}` MUST go on separate lines.

```
if ()
{
  xxx();  // 
}
else
{
}
```

Also, open and close brackets are used also for one-liners.

This said, there is an exception to this rule that makes the code really readable. Namely, a set of really short 
statements after ifs (easier to explain with an example):

```
// Stupid ctoi (char to integer) implementation:

int ctoi(char c)
{
  if (c == '0')    return 0;
  if (c == '1')    return 1;
  if (c == '2')    return 2;
  if (c == '3')    return 3;
  ...
}
```

    The same goes for 'switch/case':

```
int ctoi(char c)
{
  switch (c)
  {
  case '0':   return 0;
  case '1':   return 1;
  case '2':   return 2;
  case '3':   return 3;
  ...
  }
}
```

*Rationale*: The reason for this is to have the brackets horizontally aligned, which makes it easier to understand
where a block ends. The alternative style `if () {` gains one line which makes a lot of sense when editing books
(for printing on paper), to save one line, but it does not make much sense to save that line in a source code file
that is only viewed using editors.

*How to check*: manually

#### M9 (Switch statement):

*Rule*: Switch statements follow the same rules as other statements about `{` on separate lines etc. What differs 
about `switch` statements is that it contains labels. In a switch statement the `case`s belong to the switch and
thus they have the same indentation as the `switch`.

Very short 'case-statements' can be on the same line as the 'case-label', as seen in the previous example. Also when a 
break is needed (if return is used, break isn't really needed):

```
switch (c)
{
case '0':   i = 0; break;
case '1':   i = 1; break;
case '2':   i = 2; break;
case '3':   i = 3; break;
...
}
```
    
When longer statements are used, the 'case-label' MUST have its own line and the statements MUST have one line 
each, as normally. Also, there MUST be an empty line between 'break' and 'case', to clearly show when one 
'case-block' ends and another one starts, like this:

```
switch (c)
{
case '0':
  strncpy(out, "This would be case 0", sizeof(out));
  break;

case '1'
  ...
}
```

*Rationale*: in general, each statement on a separate line makes the code much easier to read. However, these typical very 
short switches are really much easier to read if we relax the rule a little.

*How to check*: manually

#### M10 (Command & operators separation):

*Rule*: operators (=, ==, etc). are followed and preceded by ONE space. Commas are followed by ONE space.

```
myFunction(x, y, z);
if (a == b)
{
  c = d;
}
```

not

```
myFunction(x,y,z);
if (a==b)
{
  c=d;
}
```

*Rationale*: easier on the eye.

*How to check*: checked using the internal script ```scripts/style_check.sh```

#### M11 (Spaces)

*Rule*: The keywords that are followed by '(' MUST have a space between the keywords and the parenthesis:

```
if () ...
while () ...
do () ...
switch () ...
etc
```

Function calls (or function declarations) on the other hand MUST NOT have a space between the function name and the 
parenthesis:

```
funcCall();
```

* Also, after `(` and before `)`, spaces MUST NOT be present.
* Same thing goes for arrays (`[]`). 
* All binary operators MUST be surrounded by spaces (`=`, `==`, `>=`, `<`, etc).
* Unary operators MUST NOT have any space between itself and the operand (`!`, `++`, etc).
* Conditions (in `if`, `for`, `while`, etc) MUST follow the following rules:
  * 'condition glue operators' (`&&`, `||`) MUST be preceded and superceded by space: ` && `, ` || `.
  * all operators (`==`, `>` ...) MUST be preceded and superceded by space: `if (a == 7) ...`
  * NO space after `(`, nor before `)`
* After a comma, one space, just like for normal text. The exception is when horizontal alignment helps 
  readability when commas may be followed by more than one space:

```
mongoInit(user, pwd, host);
xyzInit(user,   pwd  host);
``` 

*Rationale*: easier on the eye.

*How to check*: checked using the internal script ```scripts/style_check.sh```

#### M12 (`using namespace` in header files)

*Rule*: the construct ```using namespace XXX``` must not be used in header files.

*Rationale*: a header file should not decide for the source files that include the header file.
If a header file contains this construct then all source files that include the header file,
directly or indirectly are forced to have that construct as well.

*How to check*: using an intelligent ```grep```.

#### M12bis (`using namespace` in source files)

*Rule*: the construct `using namespace XXX` must not be used in source files (i.e. `.cpp`). The construct `using XXX::YYY` may be used in source files.

*Rationale*: Much clearer where functions/variables/types come from when `using namespace XXX` is not used.

*How to check*: checked using the internal script `scripts/style_check.sh`

### ‘SHOULD follow’ rules:

#### S1 (Statements):

*Rule*: Each statement SHOULD have its own line.

*Rationale*: easier to read the code.

*How to check*: manually

#### S2 (Object action naming convention):

*Rule*: "objectAction" SHOULD be used, eg.:

```
listInit()
listReset()
listFind()
```
instead of

```
initList()
resetList()
findList()
```

This rule is applied to variable names, function names and even file names (for files which
contain only one external function so the name of the file is the name of the function).

*Rationale*: a set of functions are ‘grouped’ thanks to their prefix (“object”, in the example: “list”).

*How to check*: manually

#### S3 (Parenthesis in Part-Conditions):

*Rule*: Conditions (in `if`, `for`, `while`, etc) SHOULD have part-conditions inside parenthesis: 

```
if ((a == 2) || (a == 7)) ...
```

*Rationale*: much easier to read, impossible to get unwanted behavior due to precedence.

*How to check*: manually

#### S4 (Function separation)

*Rule*: we add THREE empty lines before the function header to CLEARLY see where one function ends and another function starts.

*Rationale*: it is good to clearly see when one function ends and another one starts. Why three lines? Well, one 
line is not enough, three is good.

*How to check*: manually

#### S5 (Preconditions)

*Rule*: we strongly recommend for functions to evaluate the parameters and if necessary return error, 
before starting to process. 

An example:

```
// Bad implementation:
bool stringCheck(char* s1, char* s2, char* s3, char* s4)
{
  bool ret = false;

  if (s1 != NULL)
  {
    if (s2 != NULL)
    {
      if (s3 != NULL)
      {
        if (s4 != NULL)
        {
          printf("All four strings non-NULL");
          ret = true;
        }
      }
    }
  }

  return ret;
}

// Better implementation
bool stringCheck(char* s1, char* s2, char* s3, char* s4)
{
  if ((s1 == NULL) || (s2 == NULL) || (s3 == NULL) || (s4 == NULL))
  {
    return false;
  }

  printf("All four strings non-NULL");
  return true;
}
```

*Rationale*: This to avoid long nested 'if-else's that only make a function more difficult to understand.

*How to check*: manually

#### S6 (Variable align)

*Rule*: Variables SHOULD be aligned horizontally, and also, if assignments are taken place, the `=` operators 
SHOULD also be aligned:

```
int             x  = 12;
float           f  = 3.14;
struct timeval  tm;
```

Between the type name and the variable name, a TWO space separation, to clearly separate what is the type and what is the 
variable name. Sometimes the type consists of more than one word (e.g. `struct timeval`).

*Rationale*: the code gets much easier to read.

*How to check*: manually

#### S7 (Order in source/header files):

*Rule*: the following order should be used:

* `#ifndef/#define` (only if header file, obviously)
* Copyright header
* Includes
* External declarations (if applicable, only source files)
* Definitions (including macros)
* Typedefs
* Variables (external declarations if header file)
* Functions (external declarations if header file)
* `#endif` (only if header file, obviously)

*Rationale*: order, order and more order.

*How to check*: manually

#### S8 (Includes):

*Rule*: 

* The list of include files MUST start with files from the standard C library, like `<stdio.h>` or `<string.h>`
* After that, C++ files from the STL, like `<string>` or `<vector>`
* Third party header files, like `mongo/xxx.h`
* Files from other libraries of this executable
* Files from the same library (or same directory if it's an executable)
* Lastly, the corresponding header file

All C/C++ files (except possibly the file containing `main()`  of an executable) MUST have a corresponding header 
file. Example: `list.cpp` implements the functionality, while `list.h` is the external part for others to include.  

*Rationale*: Now, to explain why it is so important for a source file to include its corresponding header file: 
Imagine that `list.cpp` DOES NOT include `list.h`, and we find listInit(void) in the header file, but 
`listInit(int)` in the source file - `list.cpp` will happily compile, but when used, the users include `list.h`,
which states that `listInit` has no parameters. Result?  Disaster. Often SIGSEGV. So, to remedy this, ALL C/C++
files MUST include their corresponding header file, and this possible problem is ALWAYS detected at compilation
time.

*How to check*: manually, except order in inclusions (first std C files, then std C++ files, etc), that is checked by the internal script ```scripts/style_check.sh```

#### S9 (Line length):

*Rule*: A line SHOULD not have more than 120 characters, but if readibility is worsened by splitting the line, 
then leave the line as long as needed. Consider if the line can be shortened with the help of variables, eg.:

```
//
// Example of a long line
//
if ((contextAttributeP->metadataVector[0]->name == "") && (contextAttributeP->metadataVector[0]->type == "") && (contextAttributeP->metadataVector[0]->xyz == NULL))
{
}
            
//
// Same line made shorter using a help variable
//
Metadata* mdP = contextAttributeP->metadataVector[0];

if ((mdP->name == "") && (mdP->type == "") && (mdP->xyz == NULL))
{
}
```

*Rationale*: 120 is a reasonable limit taking into account the current visualization technologies (high 
definition monitors, resizable windows and scrollbars).

*How to check*": checked by the internal script ```scripts/style_check.sh```

#### S10 (Ternary operator):

*Rule*: The ternary operator is more than welcome, but it SHOULD NOT be nested:

```
char* boolValue = (b == true)? "true" : "false";
```

Now, if we nest it (let's pretend we have an enum with `True`, `False` and `Perhaps` as values):

```
char* xboolValue = (b == True)? "True" : (b == False)? "False" : "Perhaps";
```

Still extremely compact, but no longer very elegant and definitely not very easy to understand.
(Keep in mind this example is extremely simple - this nesting could get really ugly ...). Nested ternary operator SHOULD 
NOT be used. In this particular case a `switch` might be a better option:

```
char* boolValue;

switch (b)
{
case True:     boolValue = "True";    break;
case False:    boolValue = "False";   break;
case Perhaps:  boolValue = "Perhaps"; break;
}
```

*Rationale*: readibility.

*How to check*: manually

#### S11 (Empty lines):

*Rule*: Using empty lines between logical parts of a function is considered good practice. One line is usually
enough, no need to exaggerate. Also, after the variable declaration in the beginning of the function, please add 
an empty line before the code starts.
    
After a block `{}`, an empty line is often a good idea.

Before `return`, an empty line is pretty much always a good idea. `return` is an important instruction and 
should be perfectly visible.

As mentioned, between cases in a switch an empty line is a MUST, except for the short switch, where the `case` 
and various commands share a line.

Empty lines MUST NOT appear after `{` or before `}`.

*Rationale*: It can really help the readability of source code.

*How to check*: manually

#### S12 (C++ initialization lists):

*Rule*: C++ initialization lists should be used and it should be on the same line as the constructor unless 
the line gets too long that way. If too long a line, every initialized variable should be on its own line and 
indented with two spaces, see example:

```
X:X(int _i, float _f):  i(_i), f(_f)
{
}

Y:Y(const std::string& _fooName, const std::string& _myLongFooName):
  fooName(_fooName),
  myLongFooName(_myLongFooName)
{
}
```

*Rationale*: doing this way is more efficient than assigning values in the constructor body given that the 
values are assigned at object construction time in memory at execution time.

*How to check*: combination of compiler warning + manually

#### S13 (Pointer variables naming)

*Rule*: pointer variable names should use `P` (capital P) as suffix.

```
Entity* eP;
```

*Rationale*: pointer variables can be easily identified at a glanze, which makes the code clearer.

*How to check*: manually

#### S14 (Referenced variables in function calls)

*Rule*: parameters that are passed by "C++ reference", either `const` or not, should be a variable, not a result of a computation.
See example:

```
  extern void f(const BSONObj& bobj);

  // NOT like this:
  f(BSON("a" << 12));

  // BUT, like this:
  BSONObj bo = BSON("a" << 12);
  f(bo);

```

*Rationale*: It's simply weird to pass a value for a referenced parameter. If `const` is not used you get a compiler error,
which makes a lot of sense: how can the function modify the variable when there is no variable?
In "C", it's more straightforward, as in order to send the reference to the variable using the `&` operator, you need a variable.
In C++ it gets a a bit weird and it is better to avoid this by adding a helper variable (`bo` in the example above).
The difference between "C pointers" and "C++ references" is minimal, but really, it depends on the implementation of the compiler.
See [this question](https://stackoverflow.com/questions/44239212/how-do-c-compilers-actually-pass-literal-constant-in-reference-parameters) and 
[this one](https://stackoverflow.com/questions/2936805/how-do-c-compilers-actually-pass-reference-parameters) in stackoverflow, for discussions on this. 

*How to check*: manually

#### S15 (Error output parameter at the end)

*Rule*: in the case a function uses an output parameter to potencially provide error output to the caller, that
parameter SHOULD be declared at the end of the parameters list, e.g.:

```
+void mongoRegistrationGet
(
  ngsiv2::Registration*  regP,
  const std::string&     regId,
  const std::string&     tenant,
  const std::string&     servicePath,
  OrionError*            oeP
);

void myFunction(const std::string s, std::string* err);
```

*Rationale*: the code gets more ordered this way.

*How to check*: manually

#### S16 (Use empty() for checking zero-length strings)

*Rule*: Use empty() for all string empty-ness checking, avoiding comparisons with `""` or `length() == 0`

*Rationale*: empty() is [the best way of checking for zero-length strings](http://stackoverflow.com/questions/41425569/checking-for-empty-string-in-c-alternatives).

*How to check*: manually

## Programming patterns

Some patterns are not allowed in Orion Context Broker code, except if some strong reason justifies the use of it. 
If you plan to use some of them, please consult before with the core developers.

* Multiple inheritance. It introduces complexity in the code. It is generally accepted that it is a “problematic”
  pattern.
* C++ exceptions 
  * Exceptions (not checked in C++) make it harder to follow the flow of execution. It is hard to say whether
  a function will throw an exception, complicating refactoring and debugging.
  * Exception-safe functions requires ensuring the transactional semantic is kept, (beyond RAII to avoid resource leaks),
  making the code error-prone due to subtle interactions. It gets worse in a multithreaded environment.
  * The flavor of this project is biased to C clearly. Many plain C libraries are used and mixing exceptions and
  traditional if-checking method should be avoided as much as possible.
* Object passing. In order to avoid inefficient object copies when calling functions, the following criteria apply:
  * If the function should not be able to modify the object (i.e. "read only"), then const references should be used, e.g. `const BigFish& bf`
  * If the function must be able to modify the object (i.e. "read/write" or "write"), then a pointer type should be used, e.g: `BigFish* bf`. Note that in this case a C++ reference could be used as well, but we prefer pointers to clearly illustrate that the object might be be modified by the function.

```
void myFunction(const BigFish& in, BigFish* out)
```

* Strings return. In order to avoid inefficient object copies when returning strings, the  `const std::string&` return type is preferred. In the case this pattern cannot be used (e.g. when literal string such as "black", "red", etc. are used in the call of the function) then `const char*` should be used as return type.

```
const std::string& myFunction(...)
const char* myOtherFunction(...)
```

