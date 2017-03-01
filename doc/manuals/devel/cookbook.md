# Orion Implementaton Cookbook

## Adding a command line parameter
It's fairly easy to add a new CLI parameter to Orion, as there is a library in charge of
parsing and checking the CLI parameters.
This library (**parseArgs) is called by the main program in contextBroker.cpp as one of its first
actions. The function to parse CLI arguments is called parseArgs, and it has three parameters:

* argC - the number of arguments for the main program
* argV - the argument vector for for the main program
* paArgs - a vector describing the CLI parameter that the broker recognizes

You basically need to implement two things:

* a **variable** to hold the value of the new CLI parameter, and
* a **new item** in the PaArgument vector `paArgs`

If the new CLI parameter is a boolean one, like *-v (verbose)*, a `bool` variable is needed,
if it's a text parameter, like *-dbHost <host name>*, a char-vector is used, and so on.

The easiest way is to simply copy an older CLI parameter of the same type.

The item in the PaArgument vector `paArgs` contains nine different pieces of information:

* the name of the CLI option
* a pointer to the variable that will hold its value after parse
* the name of the environment variable (yes, options can be passed an env vars also)
* the type of the CLI parameter variable:
    * PaBool
    * PaString
    * PaInt
    * PaDouble
    * ... (see the **PaType** enum in `src/lib/parseArgs/parseArgs.h`)
* the type of the CLI parameter itself:
    * PaOpt, for **optional** parameters
    * PaReq, for **required** parameters
    * PaHid, for **hidden** parameters (not presented in **usage()**)
* default value (the value used if the parameter is not given)
* minimum value (Use **PaNL** if no minimum value is desired)
* maximum value (Use **PaNL** if no maximum value is desired)
* a descriptive string, used for the `usage()` function 

**Remember**:

* There are no minimum/maximum values for string (doesn't make much sense, does it?), so PaNL is
  always used for strings.
* The second item in the PaArgument must be a pointer, so if not a string (a char vector is a pointer),
  you have to pass the reference of the variable to hold the value (&x).
* The sixth item of the PaArgument (the default value) is an integer (long long), so if the default value
  is a string, it needs to be typecast to an integer. There is a special macro (**_i**) for this.


### Example: adding an integer CLI parameter -xyz
As a hands-on example, lets add an integer CLI parameter, called **-xyz**:  

Edit `src/app/contextBroker/contextBroker.cpp, and look for an already existing integer CLI parameter.

1. Create the integer variable `xyz`, right where 'int port' is (search for 'Option variables').

1. Add the PaArgument line for 'xyz':
    Search for `PaArgument paArgs[]`.
    Inside that vector, look for a vector item that has PaInt as fourth item - we find the parameter **-port**:
    
      `{ "-port", &port, "PORT", PaInt, PaOpt, 1026, PaNL, PaNL, PORT_DESC },`

    Copy that line, and in the copied line, change all 'port' for 'xyz', end up seeing this:

    ```    
       { "-port", &port, "PORT", PaInt, PaOpt, 1026, PaNL, PaNL, PORT_DESC },  
        { "-xyz",  &xyz,  "XYZ",  PaInt, PaOpt, 1026, PaNL, PaNL, XYZ_DESC  },
    ```    
1. Create the `XYZ_DESC` description string, right after `PORT_DESC`.
1. If xyz is a *required option*, change **PaOpt** for **PaReq**, or **PaHid** if it is to be hidden.
1. Change the **1026** for the default value for xyz, e.g. 47
1. Set the minimum and maximum values of xyz (items 7 and 8 in the PaArgument line).
1. Compile the broker (make di)
1. Run: `contextBroker -u` and you should see (unless **PaHid** was used):  
    `[option '-xyz <description of xyz>]`
1. Run: `contextBroker -U` and you will see more information about the CLI parameters,
   including default values, min and max values, etc.
1. If you gave -xyz any min/max limits, try starting the broker with invalid values and see it complain.
1. If you made -xyz **PaReq**, try starting the broker without -xyz and see what happens.
1. If you made -xyz **PaHid**, make sure it **is not visible** running `contextBroker -u`


A note about environment variables as options:  
*All environment variables are prefixed "ORION_" (see the call to `paConfig("builtin prefix", ...)`), so if you
supply an environment variable name name of "XYZ" as the third item in the PaArgument vector-item, then the
complete name of the environment variable is ORION_XYZ*


## Adding a REST service
