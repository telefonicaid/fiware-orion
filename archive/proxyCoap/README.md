This directory holds proxyCoap, no longer mantained as part of the "mainstream" code in Orion (the one
under the src/ directory at repository root) but maybe useful in the future.

## Building

This directory cannot be built in "standalone mode" (as the archiving work described at issue #1202 is not
completed), so by the moment if you want to compile proxyCoap you have to move the
proxyCoap/src/app/proxyCoap directory to its original location at src/app in the repository root, then add
the following to root CMakeLists.txt file:

```
ADD_SUBDIRECTORY(src/app/proxyCoap)
```

just after the line:

```
ADD_SUBDIRECTORY(src/app/contextBroker)
```

In addition, you need to do the following before running `make` to compile:

* Install cantcoap (with dependencies). Note that we are using a particular snapshot of the code (corresponding
  to around July 21st, 2014) given that cantcoap repository doesn't provide any releasing mechanism.


```
sudo yum install clang CUnit-devel

git clone https://github.com/staropram/cantcoap
cd cantcoap
git checkout 749e22376664dd3adae17492090e58882d3b28a7
make
sudo cp cantcoap.h /usr/local/include
sudo cp dbg.h /usr/local/include
sudo cp nethelper.h /usr/local/include
sudo cp libcantcoap.a /usr/local/lib
```


## Testing

The cases/ directory contains a set of test harness files used in the past to test the
proxyCoap functionality. You need to install COAP client (an example application included in the
libcoap sources) before to use them:

```
wget http://sourceforge.net/projects/libcoap/files/coap-18/libcoap-4.1.1.tar.gz/download
mv download libcoap-4.1.1.tar.gz
tar xvzf libcoap-4.1.1.tar.gz
cd libcoap-4.1.1
./configure
make
sudo cp examples/coap-client /usr/local/bin
```
