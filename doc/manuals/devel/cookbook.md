# <a name="top"></a>Orion Implementaton Cookbook

* [Adding a command line parameter](#adding-a-command-line-parameter)
* [Adding a REST service](#adding-a-rest-service)
* [Adding a Functional Test Case](#adding-a-functional-test-case)
* [Catching a '405 Method Not Allowed'](#catching-a-405-method-not-allowed)
* [Fixing a memory leak](#fixing-a-memory-leak)

## Adding a command line parameter
It's fairly easy to add a new [CLI parameter](../admin/cli.md) to Orion, as there is a library in charge of parsing and checking the CLI parameters. This library ([**parseArgs**](sourceCode.md#srclibparseargs)) is called by [the main program](sourceCode.md#srcappcontextbroker) in `contextBroker.cpp` as one of its first actions. The function to parse CLI arguments is called `parseArgs()`, and it has three parameters:

* `argC`, the number of arguments for the main program
* `argV`, the argument vector for for the main program
* `paArgs`, a vector describing the CLI parameter that the broker recognizes

You basically need to implement two things:

* a **variable** to hold the value of the new CLI parameter, and
* a **new item** in the `PaArgument` vector `paArgs`

If the new CLI parameter is a boolean one, like `-v` (verbose), a `bool` variable is needed,
if it's a text parameter, like `-dbHost <host name>`, a char-vector is used, and so on.

The easiest way is to simply copy an older CLI parameter of the same type.

The item in the `PaArgument` vector `paArgs` contains nine different pieces of information:

* the name of the CLI option
* a pointer to the variable that will hold its value after parse
* the name of the environment variable (yes, options can be passed as env vars also)
* the type of the CLI parameter variable:
    * `PaBool`
    * `PaString`
    * `PaInt`
    * `PaDouble`
    * ... (see the `PaType` enum in `src/lib/parseArgs/parseArgs.h`)
* the type of the CLI parameter itself:
    * `PaOpt`, for **optional** parameters
    * `PaReq`, for **required** parameters
    * `PaHid`, for **hidden** parameters (not presented in `usage()`)
* default value (the value used if the parameter is not given)
* minimum value (Use `PaNL` if no minimum value is desired)
* maximum value (Use `PaNL` if no maximum value is desired)
* a descriptive string, used for the `usage()` function 

**Remember**:

* Boolean CLI parameters can only take two possible values: `true` or `false`. No value is added on command line, just the option itself, e.g.:   `-fg` as opposed to `-port <port number>`.
* There are no minimum/maximum values for string (it wouldn't make much sense), so `PaNL` is always used for strings.
* The second item in the `PaArgument` must be a pointer, so if not a string (a char vector is a pointer), you have to pass the reference of the variable to hold the value (`&x`).
* The sixth item of the `PaArgument` (the default value) is an integer (`long long`), so if the default value is a string, it needs to be typecast to an integer. There is a special macro (`_i`) for this.


### Example: adding an integer CLI parameter `-xyz`
As a hands-on example, lets add an integer CLI parameter, called `-xyz`.

Edit `src/app/contextBroker/contextBroker.cpp`, and look for an already existing integer CLI parameter.

1. Create the integer variable `xyz`, right where `int port` is (search for `Option variables` comment).

2. Add the `PaArgument` line for xyz:
    * Search for `PaArgument paArgs[]`.
    * Inside that vector, look for a vector item that has `PaInt` as fourth item: we find the parameter `-port`:
    
      `{ "-port", &port, "PORT", PaInt, PaOpt, 1026, PaNL, PaNL, PORT_DESC },`

    * Copy that line, and in the copied line, change all 'port' for 'xyz', end up seeing this:

    ```
       { "-port", &port, "PORT", PaInt, PaOpt, 1026, PaNL, PaNL, PORT_DESC },
       { "-xyz",  &xyz,  "XYZ",  PaInt, PaOpt, 1026, PaNL, PaNL, XYZ_DESC  },
    ```    
3. Create the `XYZ_DESC` description string, right after `PORT_DESC`.
4. If xyz is a *required option*, change `PaOpt` for `PaReq`, or `PaHid` if it is to be hidden.
5. Change the `1026` for the default value for xyz, e.g. `47`
6. Set the minimum and maximum values of xyz (items 7 and 8 in the `PaArgument` line).
7. Compile the broker (`make debug install`)
8. Run: `contextBroker -u` and you should see (unless `PaHid` was used):  
    `[option '-xyz <description of xyz>]`
9. Run: `contextBroker -U` and you will see more information about the CLI parameters,
   including default values, min and max values, etc.
10. If you gave `-xyz` any min/max limits, try starting the broker with invalid values and see it complain.
11. If you made `-xyz` `PaReq`, try starting the broker without `-xyz` and see what happens.
12. If you made `-xyz` `PaHid`, make sure it **is not visible** running `contextBroker -u`


A note about environment variables as options:

* The builtin environment variables are prefixed `ORION_` (see the call to `paConfig("builtin prefix", ...)`), so builtin CLI options such as `-t, `-logDir`, etc get that prefix for their env vars.
if you supply an environment variable name of `XYZ` as the third item in the `PaArgument` vector-item, then please respect the prefix and call it `ORION_XYZ`.
That said, we've never really used env vars for the options and the `ORION_` prefix is not respected by the current implementation. This is a bit unfortunate but really easy to fix (not counting breaking backward-compatibility, of course ;-)).
However, try the `-U` CLI option to see the env vars and more info for each of the CLI options.

To try setting a CLI option via env vars, execute this as a test:
```
% export FOREGROUND=1
% contextBroker -U  # UPPERCASE U !
Extended Usage: contextBroker  [option '-U' (extended usage)]                       TRUE /FALSE/  (command line argument)
...
                               [option '-fg' (don't start as daemon)]  FOREGROUND   TRUE /FALSE/  (environment variable)
...
% unset FOREGROUND
```
Note the right-most column saying `(environment variable)` for the `-fg` option. This indicates that the value for `-fg` has been taken from its environment variable (`FOREGROUND`) and as long as `FOREGROUND` exists (is not unset), Orion will start in foreground.

[Top](#top)

## Adding a REST service
The REST services that the Orion Context Broker supports are items in the seven `RestService` vectors `restServiceV`, found in [orionRestServices.cpp](sourceCode.md#srcappcontextbroker). There is one service vector per HTTP Method/Verb that Orion supports: GET, PUT, POST, PATCH, DELETE, OPTIONS, plus a special vector for 'bad verb'. The set of services that are supported pretty much defines the role and by starting the REST interface with one `RestService` vector or another defines what the broker is able to do, all its services are included in these seven vectors.

To add a REST service to Orion, a new item in `RestService xxxServiceV[]` (`xxx` being the verb of the service (`get`, `put`, etc) is needed. Just like with CLI parameters, the easiest way is to copy an old service (an item in `xxxServiceV`) and then modify the copy to suit your needs.

To understand this new item in the `RestService` vector, take a look at the struct `RestService`, in `src/lib/rest/RestService.h`:

```
typedef struct RestService  
{  
  RequestType   request;          // The type of the request  
  int           components;       // Number of components in the URL path  
  std::string   compV[10];        // Vector of URL path components. E.g. { "v2", "entities" }  
  std::string   payloadWord;      // No longer used, should be removed ... ?  
  RestTreat     treat;            // service function pointer  
} RestService;
```

So, to add a REST service eg. `PUT /v2/entities/{EntitId}/attrs/{AttributeName}/metadata/{MetadataName}`, the new item if the RestService vector `putServiceV` would look like this:

```
{ Metadata,  7, { "v2", "entities", "*", "attrs", "*", "metadata", "*" }, "", putMetadata }
```

NOTE:

* Item 1: `Metadata` would have to be added as an enum constant in the `enum RequestType` in `src/lib/ngsi/Request.h`
* Item 3: `"*"`. An asterisc in the component vector `RestService::compV` matches ANY string, and whenever a path including entity id, attribute name, etc is defined, `"*"` must be used.
* Item 5: `putMetadata()` is the service routine for `PUT /v2/entities/*/attrs/*/metadata/*` and the function must be implemented. The directory of the library for NGSIv2 service routines is `src/lib/serviceRoutinesV2` (see [library description](sourceCode.md#srclibserviceroutinesv2)).

Note also that in `orionRestServices.cpp`, these `RestService` vector lines are really long, and our style guide is against too long lines. However, making the lines shorter by using definitions just make the code more difficult to understand and we don't want that.

> Side-note: The [style guide](../contribution_guidelines.md#s9-line-length) says a source code line **shouldn't** be longer than 120 chars.

The service routine `putMetadata()` should reside in `src/lib/serviceRoutinesV2/putMetadata.h/cpp` and its signature must be as follows:  
```
std::string putMetadata  
(  
  ConnectionInfo*            ciP,  
  int                        components,  
  std::vector<std::string>&  compV,  
  ParseData*                 parseDataP  
)  
```

The `entity id`, `attribute name`, and `metadata name` (all part of the URL path), must be "extracted" from the component vector `compV`:

```
  std::string entityId      = compV[2];  
  std::string attributeName = compV[4];  
  std::string metadataName  = compV[6];  
```

All service routines that modify/create entities/attributes/metadata rely on the NGSIv1 service routine `postUpdateContext()`, and `putMetadata()` is no exception. So, what needs to be done in `putMetadata()` is to build a `UpdateContextRequest` object using the parameters of `putMetadata()` and call `postUpdateContext()`. Something like this:

```
  parseDataP->upcr.res.fill(entityId, attributeName, metadataName, "APPEND");  
  postUpdateContext(ciP, components, compV, parseDataP, NGSIV2_FLAVOUR_ONAPPEND);    
```

`UpdateContextRequest` has a bunch of `fill()` methods (seven `fill()` methods as of March 2017) and if there is no fill-method suited for your demands in `putMetadata()`, then another fill-method must be implemented for `UpdateContextRequest`.

It is easy enough, just copy from an older, similar, fill-method.

Now just add `putMetadata.cpp` to the CMake file `src/lib/serviceRoutinesV2/CMakeLists.txt` and compile the broker. To test that `putMetadata()` works correctly, a new functional test case should be implemented. [The following recipe](#adding-a-functional-test-case) explains how to do that.

To capture "POST/PATCH/XXX /v2/entities/*/attrs/*/metadata/*" and respond with a `405 Method Not Allowed`, please have a look at [the recipe about bad method](#catching-a-405-method-not-allowed).

[Top](#top)

## Adding a Functional Test Case
The functional tests of Orion are text files with the suffix `.test` and reside in `test/functionalTest/cases/{case-dir}`. The "case directories" are named after the github issues.

As always, the easiest way to implement a new functional test is to "steal" from older ones.  

A functional test file contains six sections:

1. Copyright section
2. NAME section
3. SHELL-INIT section
4. SHELL section
5. EXPECT/REGEXPECT section
6. TEARDOWN section

Each section (except the Copyright preamble, that starts at the beginning of the file) must have a header, that tells the functional test harness where every section starts/ends:

* `--NAME--`
* `--SHELL-INIT--`
* `--SHELL--`
* `--REGEXPECT--` / `--EXPECT--`
* `--TEARDOWN--`

If `--REGEXPECT--` is used (and not `--EXPECT--`), then the expected section permits regular expressions. That is the only different between these two.

### Copyright section
This section is simply for the Copyright header. Copy an old one. Try to remember to change the year, if necessary.

### NAME Section
Simply put the name of the test in this section:

```
--NAME--  
Example Test Case
```

### SHELL-INIT Section
This is where initialization tasks are performed.
Like:

* Wiping out data bases
* Starting the broker
* Starting context providers
* Starting the accumulator

Example (normal case):

```
--SHELL-INIT--  
dbInit CB
brokerStart CB
accumulatorStart
```

Example with broker and five context providers ("stolen" from the existing test case `test/functionalTest/cases/1016_cpr_forward_limit/fwd_query_limited.test`):

```
--SHELL-INIT--  
dbInit CB  
dbInit CP1  
dbInit CP2  
dbInit CP3  
dbInit CP4  
dbInit CP5  
brokerStart CB 0 IPV4 "-cprForwardLimit 3"  
brokerStart CP1  
brokerStart CP2  
brokerStart CP3  
brokerStart CP4  
brokerStart CP5  
```

### SHELL Section
The broker is started in the SHELL-INIT section and this section is where curl commands (and other commands) are executed to send requests to Orion and perform the functional test.  

A shell function called `orionCurl` is implemented for the shell section to be easier to read and implement. The implementation of `orionCurl`, and many other help functions is found in `test/functionalTest/harnessFunctions.sh`.

Note that each step in the Shell section starts with a short descriptive header, like this:

```
echo "0x. description of test step 0x"  
echo "==============================="  
```

and the steps end with two calls to `echo`, to separate the current step from the next in the output. This is pretty important as it makes it **so much** easier to read the output, which must match what is in the section that follows, the **EXPECT/REGEXPECT** section.

A typical step (e.g. to create an entity) looks like this:  

```
echo "01. Create entity E1 with attribute A1"  
echo "======================================"  
payload='{  
  "id": "E1",  
  "type": "T1",  
  "A1": {  
    "value": 1,  
    "type": "Integer",  
    "metadata": {  
      "md1": {  
        "value": 14  
      }  
    }  
  }  
}'  
orionCurl --url /v2/entities --payload "$payload"  
echo  
echo
```

### EXPECT/REGEXPECT Section
First of all, the test harness (`test/functionalTest/testHarness.sh`) admits two types of 'expect sections'. Either

```
--EXPECT--
```

or

```
--REG-EXPECT--
```

You have to **pick one**. Pretty much **all** current functests use the `--REG-EXPECT--` type. The advantage with --REG-EXPECT-- is that it permits to add regular expressions using the `REGEX()` syntax, which is very important for the comparison of dates, or IDs created by Orion and returned in the response, like a registration id or a correlator or a simple timestamp. An important limitation is that there can only be **one REGEX** per line in the REG-EXPECT section.

That said, in the REG-EXPECT section, just add what is the expected output from the test step in question. For example, the example "01. Create entity E1 with attribute A1" from the above sub-chapter about the SHELL section would
have this corresponding piece in the --REGEXPECT-- section:  

```
01. Create entity E1 with attribute A1  
======================================  
HTTP/1.1 201 Created  
Content-Length: 0  
Location: /v2/entities/E1?type=T1  
Fiware-Correlator: REGEX([0-9a-f\-]{36})  
Date: REGEX(.*)  
  
  
  
```

Note that after two first lines, what comes out from `orionCurl` is first the HTTP headers, and after that eventual payload. In this example there is no payload.

Note the two occurrences of `REGEX()`, for the correlator and the date:

* The correlator is a string of 36 characters, that is a hex number with hyphens. This regex could be made better, now that we know exactly
where each hyphen must come, however, it's not really necesary.  
* The second REGEX, for the `Date` HTTP header could also be more elaborated. Also not necessary.

### TEARDOWN Section
This is where processes are killed and databases are removed, so that the following test case will start with a clean slate. The most typical commands used are:

```
--TEARDOWN--  
brokerStop CB  
dbDrop CB  
```

If the accumulator is used, or context providers, those must be stopped also:

```
accumulatorStop  
brokerStop CP1  
brokerStop CP2  
```

Note that in the functional tests, we start instances of Orion to act as context providers. The log file directory and the port number, etc. are changed for the instances acting as context providers. See `scripts/testEnv.sh` for the variables `CP1_PORT`, `CP2_PORT` etc. ]

And, the databases (tenants) must be wiped out:

```
dbDrop CP1  
dbDrop CP2  
```

If tenants are used with Orion running just as Orion (as opposed to a context provider):

```
orionCurl --tenant T1 --url /v2/entities --payload "$payload"  
```

then the tenant T1 (database name ftest-T1) must be wiped out as well:

```
dbDrop t1  
```

Note that `t1` is used and not `T1`. This is because Orion converts tenants to all lowercase.

[Top](#top)

## Catching a '405 Method Not Allowed'
Orion supports the requests

* `GET /v2/entities/{EntityId}` AND
* `DELETE /v2/entities/{EntityId}`,

but, what happens if a `POST /v2/entities/{EntityId}` is issued to the broker?

Well, normally (as `POST /v2/entities/{EntityId}` is not supported), a `404 Not Found` would be the result. However, as Orion catches ANY method for the URL `/v2/entities/{EntityId}` with the service routine `badVerbGetDeleteOnly()`, Orion is able to respond with a `405 Method Not Allowed` - the URL is OK, but the verb/method is not supported.

Please enter `contextBroker.cpp` and search for this section:

```
  #define API_V2                                                                                       \  
  { "GET",    EPS,          EPS_COMPS_V2,         ENT_COMPS_WORD,          entryPointsTreat         }, \  
  { "*",      EPS,          EPS_COMPS_V2,         ENT_COMPS_WORD,          badVerbGetOnly           }, \  
                                                                                                       \  
  { "GET",    ENT,          ENT_COMPS_V2,         ENT_COMPS_WORD,          getEntities              }, \  
  { "POST",   ENT,          ENT_COMPS_V2,         ENT_COMPS_WORD,          postEntities             }, \  
  { "*",      ENT,          ENT_COMPS_V2,         ENT_COMPS_WORD,          badVerbGetPostOnly       }, \  
  
  { "GET",    IENT,         IENT_COMPS_V2,        IENT_COMPS_WORD,         getEntity                }, \  
  { "DELETE", IENT,         IENT_COMPS_V2,        IENT_COMPS_WORD,         deleteEntity             }, \  
  { "*",      IENT,         IENT_COMPS_V2,        IENT_COMPS_WORD,         badVerbGetDeleteOnly     }, \  
```

The last three lines are the interesting ones.  

Before this section, these definitions are made:

```
  #define IENT                    EntityRequest  
  #define IENT_COMPS_V2           3, { "v2", "entities", "*" }  
  #define IENT_COMPS_WORD         ""  
```

So, as you can see:

* If a request with the URL path `/v2/entities/{EntityId}`, and the method `GET` enters the broker, then the service routine `getEntity()` takes care of the request. 
* If the method is instead "DELETE", then `deleteEntity()` takes care of the request.
* In the case of any other verb (POST, PUT, etc), `badVerbGetDeleteOnly()` takes care of the request. When `badVerbGetDeleteOnly()` takes care of the request, the response comes as `405 Method Not Allowed` and the HTTP header `Allow: GET, DELETE` is included in the response.

[Top](#top)

## Fixing a memory leak
Memory leaks are detected using [valgrind memcheck](http://valgrind.org/docs/manual/mc-manual.html). A special shell script `test/valgrind/valgrindTestSuite.sh` has been developed for this purpose and a make step is linked to it: `make valgrind`.

If `valgrindTestSuite.sh` is run by hand, remember that Orion must be compiled in DEBUG mode for it to work (`make debug install`).  

The output of the valgrind run is saved to a file with the same name as the test case, but with the suffix `valgrind.out`.  

Normally, the broker has no memory leaks, so to make an exercise **with** a memory leak, we'll have to temporarily add one:

* Open the file `src/lib/ngsi10/UpdateContextRequest.cpp` in your favorite editor
* Find the method `UpdateContextRequest::release()` and comment the call to `contextElementVector.release()`:
  ```
  void UpdateContextRequest::release(void)  
  {  
    // contextElementVector.release();  
  }  
  ```
* Recompile the broker:
  ```
  make debug install
  ```
* Run the valgrind test for a test case that uses `UpdateContextRequest` to see the leak:
  ```
  % valgrindTestSuite.sh -filter in_out_formats.test

  Test 001/1: 0000_content_related_headers/in_out_formats  ..... FAILED (lost: 2000). Check in_out_formats.valgrind.out for clues

  1 tests leaked memory:
    001: 0000_content_related_headers/in_out_formats.test (lost 2000 bytes, see in_out_formats.valgrind.out)
  ```
* Open the file `test/functionalTest/cases/0000_content_related_headers/in_out_formats.valgrind.out`
* Search for the string "definitely lost":
```
==19688== 2,000 (544 direct, 1,456 indirect) bytes in 4 blocks are definitely lost in loss record 313 of 318  
==19688==    at 0x4A075FC: operator new(unsigned long) (vg_replace_malloc.c:298)  
==19688==    by 0x6EABD4: contextElement(std::string const&, std::string const&, ParseData*) (jsonUpdateContextRequest.cpp:50)  
==19688==    by 0x6A7CF7: treat(ConnectionInfo*, std::string const&, std::string const&, JsonNode*, ParseData*) (jsonParse.cpp:180)  
==19688==    by 0x6A9935: jsonParse(ConnectionInfo*, std::pair<std::string const, boost::property_tree::basic_ptree<std::string, std::string, std::less<std::string> > >&, std::string const&, JsonNode*, ParseData*) (jsonParse.cpp:376)  
==19688==    by 0x6AA0BF: jsonParse(ConnectionInfo*, std::pair<std::string const, boost::property_tree::basic_ptree<std::string, std::string, std::less<std::string> > >&, std::string const&, JsonNode*, ParseData*) (jsonParse.cpp:416)  
==19688==    by 0x6AA94A: jsonParse(ConnectionInfo*, char const*, std::string const&, JsonNode*, ParseData*) (jsonParse.cpp:532)  
==19688==    by 0x6A3E6F: jsonTreat(char const*, ConnectionInfo*, ParseData*, RequestType, std::string const&, JsonRequest**) (jsonRequest.cpp:232)  
==19688==    by 0x688C42: payloadParse(ConnectionInfo*, ParseData*, RestService*, JsonRequest**, JsonDelayedRelease*, std::vector<std::string, std::allocator<std::string> >&) (RestService.cpp:122)  
==19688==    by 0x68B112: restService(ConnectionInfo*, RestService*) (RestService.cpp:543)  
==19688==    by 0x67E8E7: serve(ConnectionInfo*) (rest.cpp:561)  
==19688==    by 0x683E4A: connectionTreat(void*, MHD_Connection*, char const*, char const*, char const*, char const*, unsigned long*, void**) (rest.cpp:1550)  
==19688==    by 0x850B78: call_connection_handler (connection.c:1584)  
```

Now, looking at stack frame #2, the leak seems to come from a call to `contextElement()` in `jsonUpdateContextRequest.cpp`, line 50 (the exact line could be slightly different in your case). We already know why we have this leak, as we've commented a call to `ContextElementVector::release()` in `UpdateContextRequest::release()`, but  one thing is where the allocation is done, and another thing (sometimes a very different thing), is where the allocted object should be freed.

That is the tricky part of fixing leaks, knowing where the call to free/delete should be made. It is often obvious, but not always. It is not rare, when trying to fix a leak, to release an allocated buffer too soon, i.e. before it is used for the last time, so it is very important to make sure that all functional tests are fully working once all leaks are fixed. Imagine this leak found in `jsonUpdateContextRequest.cpp`, if we release the buffer right after it is allocated, then somewhere  between there and `ContextElementVector::release()`, the buffer will be used and we will most probably experience a SIGSEGV.

[Top](#top)
