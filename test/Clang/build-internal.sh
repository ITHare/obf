#release
gcc -O3 -DNDEBUG -o obfrel -std=c++1z -lstdc++ ../internal.cpp ../../src/impl/obfuscate.cpp

#debug
gcc -o obfdbg -std=c++1z -lstdc++ ../internal.cpp ../../src/impl/obfuscate.cpp

