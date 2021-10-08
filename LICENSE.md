# ADK ThirdParty Software Licenses

This file documents all of the third party software used in the ADK. Third party software is used in the ADK under three different scenarios:

* End User Deployment - artifacts from this software are deployed in some format (machine executable or otherwise) to the end-users device (set top boxes, game consoles, desktop computers, phones or tablets etc). They are also deployed in human readable or otherwise to licensees of the ADK, distribution partners etc.
    * Software at this level is restricted to the following licenses:
        * MIT [https://opensource.org/licenses/MIT]
        * BSD 3-clause [https://en.wikipedia.org/wiki/BSD_licenses]
        * MPL 2.0 [https://www.mozilla.org/en-US/MPL/2.0/]
        * zlib [https://www.zlib.net/zlib_license.html]
		* APACHE 2.0 [https://www.apache.org/licenses/LICENSE-2.0.html]
* QA - artifacts from this software will be deployed to systems used for automated builds and quality assurance testing internally at Disney companies or affiliats or otherwise licensed partners of the ADK.
    * Licene type restrictions:
        * TBD
* Development - artifacts of third party software in this category only show up in debug builds which are typically found on development tools and hardware.
    * Licene type restrictions:
        * TBD

# Development

* Meow Hash (0.5/calico) (PC Only) [https://github.com/cmuratori/meow_hash]
	* zlib [https://github.com/cmuratori/meow_hash/blob/master/LICENSE] 
* hashdict.c [https://github.com/exebook/hashdict.c]
    * MIT [https://github.com/exebook/hashdict.c/issues/1]
* cmocka (1.0.0) [https://git.cryptomilk.org/projects/cmocka.git/]
    * APACHE 2.0 [https://git.cryptomilk.org/projects/cmocka.git/tree/COPYING]
* OpenFBX [https://github.com/nem0/OpenFBX]
    * MIT [https://github.com/nem0/OpenFBX/blob/master/LICENSE]
* SQLite (3.34.1) [https://sqlite.org]
    * Public Domain [https://sqlite.org/copyright.html]
* zip_iterator.h [https://bitbucket.org/IRSmoh/zip_iterator/src/master/zip_iterator/zip_iterator.h]
    * MIT [https://bitbucket.org/IRSmoh/zip_iterator/src/master/LICENSE.md]
	
### Development binary tools

* glslcc [https://github.com/septag/glslcc]
    * BSD-2 [https://github.com/septag/glslcc/blob/master/LICENSE]
* glslangValidator [https://github.com/KhronosGroup/glslang/tree/master-tot]
    * BSD-3 [https://github.com/KhronosGroup/glslang/blob/master-tot/LICENSE.txt]

### Example assets
* canvas-experimental fbx conversion/support test files:
    * 'unity skeleton' path: "tests/private/experimental/sample_fbx/unity skeleton" [https://assetstore.unity.com/packages/3d/characters/humanoids/fantasy-monster-skeleton-35635]
        * Extension Asset [https://unity3d.com/legal/as_terms]
    * 'a.FBX' path: "tests/private/experimental/sample_fbx" [https://github.com/nem0/OpenFBX]
        * MIT [https://github.com/nem0/OpenFBX/blob/master/LICENSE]
    * 'b.fbx' path: "tests/private/experimental/sample_fbx" [https://github.com/nem0/OpenFBX]
        * MIT [https://github.com/nem0/OpenFBX/blob/master/LICENSE]
    * 'c.FBX' path: "tests/private/experimental/sample_fbx" [https://github.com/nem0/OpenFBX]
        * MIT [https://github.com/nem0/OpenFBX/blob/master/LICENSE]
    * 'd.FBX' path: "tests/private/experimental/sample_fbx" [https://github.com/nem0/OpenFBX]
        * MIT [https://github.com/nem0/OpenFBX/blob/master/LICENSE]

# End User Deployment
* Desktop
    * GLFW (3.3.1) [https://www.glfw.org/]
		* zlib/libpng [https://www.glfw.org/license.html]

* All platforms:
	* libcurl (7.73.0) [https://curl.haxx.se/]
		* MIT [https://curl.haxx.se/docs/copyright.html]
	* mbetls (2.16.5) [https://tls.mbed.org/]
		* APACHE 2.0 [https://www.apache.org/licenses/LICENSE-2.0.html]
    * Wasm3 WebAssembly interpreter [https://github.com/wasm3/wasm3]
        * MIT [https://github.com/wasm3/wasm3/blob/master/LICENSE] 
    * WebAssembly Micro Runtime [https://github.com/bytecodealliance/wasm-micro-runtime]
        * APACHE 2.0 [https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/LICENSE]
    * cJSON (1.7.12) [https://github.com/DaveGamble/cJSON]
        * MIT [https://github.com/DaveGamble/cJSON/blob/master/LICENSE]
    * libwebsockets (4.0.0) [https://github.com/warmcat/libwebsockets]
	    * MIT [https://github.com/warmcat/libwebsockets/blob/master/LICENSE]
    * nanojpeg [https://svn.emphy.de/nanojpeg/trunk/nanojpeg/nanojpeg.c]
        * MIT [https://svn.emphy.de/nanojpeg/trunk/nanojpeg/nanojpeg.c]
    * xoroshiro256++ [http://prng.di.unimi.it/xoshiro256plusplus.c]
        * CC0 1.0 [http://prng.di.unimi.it/xoshiro256plusplus.c]
    * splitmix64 [https://github.com/svaarala/duktape/blob/master/misc/splitmix64.c]
        * CC0 1.0 [https://github.com/svaarala/duktape/blob/master/misc/splitmix64.c]
    * base64encode [http://web.mit.edu/freebsd/head/contrib/wpa/src/utils/base64.c]
        * BSD [http://web.mit.edu/freebsd/head/contrib/wpa/src/utils/base64.c]
    * strcasestr [https://android.googlesource.com/platform/bionic/+/ics-mr0/libc/string/strcasestr.c]
        * MIT [https://android.googlesource.com/platform/bionic/+/ics-mr0/libc/string/strcasestr.c]
