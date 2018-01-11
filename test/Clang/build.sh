#release
gcc -O3 -DNDEBUG -DITHARE_OBF_INTERNAL_DBG -o obfrel -std=c++1z -lstdc++ ../main.cpp ../../src/impl/obfuscate.cpp

#debug
gcc -DITHARE_OBF_INTERNAL_DBG -o obfdbg -std=c++1z -lstdc++ ../main.cpp ../../src/impl/obfuscate.cpp

