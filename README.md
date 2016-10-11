# rextensions

Experiment with extending a native executable.

Works in three stages:

1. Known procedures and data structures are located by searching for patterns (See dllmain.cpp)
2. Functions are bound to proxy classes, through which the native compiled functions can be called (See examples in ProxyImpl.h)
3. Native functions are hooked through replacement functions, and new functionality is implemented (Hook.cpp)
