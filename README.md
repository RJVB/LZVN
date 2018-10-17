LZVN
====

You can download the source code of LZVN and pre-compiled copies of libFastCompression.a and libFastCompression-Kernel.a with:

``` sh
curl -o LZVN.zip https://codeload.github.com/Piker-Alpha/LZVN/zip/master
```


Unzip
-----

You can unzip the downloaded archive with:

```
unzip -qu LZVN.zip [-d target directory]
```


Make instructions
-----------------

```
make
```

on Mac, or

```
make -f Makefile.unix
```


Note: You don't need to run: ```make clean``` first because that is done automatically.


Installation
------------

You can install the executable with:

```
make install
```

or (on other Unix variants):

```
make -f Makefile.unix install
```


Note: The executable will be copied to: /usr/local/bin


Alternatively you can use cmake:

```
mkdir build
cd build
cmake ..
make
make install
```

This should work on Mac and other Unix variants. In addition this will build a shared and a static version of the FastCompression library.

Usage
-----

```
./lzvn <uncompressed filename> <compressed filename>
./lzvn -d <compressed filename> <uncompressed filename>
```

On Mac OS there are additional options:

```
./lzvn -d <path/prelinkedkernel> kernel
./lzvn -d <path/prelinkedkernel> dictionary
./lzvn -d <path/prelinkedkernel> kexts
./lzvn -d <path/prelinkedkernel> list
```

The kernel argument will extract the kernel from the given prelinkedkernel.
The dictionary argument will extract the dictionary containing the Info.plist of all kexts.
The kexts argument will extract all kexts to a ./kexts folder.
The list argument will show a list of all the included kexts.


Bugs
----

All possible bugs (so called 'issues') should be filed at:

https://github.com/Piker-Alpha/LZVN/issues

Please do **not** use my blog for this. Thank you!
