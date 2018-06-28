# Directory Structure

Orion Context Broker is divided into a number of libraries, each library
containing a number of modules (with *module* we refer to a source code file
and its corresponding header file). Each library has its own directory under `src/lib/`.

The main program, that basically initializes the libraries and starts the
REST interface resides in `contextBroker.cpp` in its own directory `src/app/contextBroker/`.

Unit tests and functional tests reside under the `test/` directory while
scripts used for testing and release making are found under `scripts/`.

* **src**: contains the source code, with the following subdirectories:
  * **app**: contains the source code for applications (each application in a separate subdirectory). The main application, the Orion Context Broker, resides in the **contextBroker** directory
  * **lib**: contains source code libraries (each library in a separate subdirectory)
* **test**: contains tests. There are several subdirectories (each subdirectory corresponding to a different test 
  suite/procedure), but the most important ones are: 
  * **unittest**: contains unit tests
  * **functionalTest**: contains functional end-to-end tests based on the "test harness" engine
  * **acceptance**: contains functional end-to-end test based on the Behave (NGSIv2) or Lettuce (NGSIv1) Python frameworks
* **scripts**: contains utility scripts (e.g. scripts included in the Orion RPM along with the Orion binary itself,
  scripts used by the test frameworks, etc.)
* **doc**: contains documentation, with the following subdirectories:
  * **apiary**: for apiary-based documentation
  * **manuals**: for markdown based documentation
* **rpm**: contains files for RPM building
* **etc**: scripts that are installed under etc/ (typically, included in RPM package)
* **docker**: contains the docker files
* **ci**: contains files required by Orion's CI process
* **archive**: contains older files that is no longer in use but that we don't feel comfortable removing just yet
