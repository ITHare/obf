#release
gcc -O3 -DNDEBUG -DITHARE_OBF_SEED=1234567 -DITHARE_OBF_DBG_RUNTIME_CHECKS -o obfofficial -std=c++1z -lstdc++ ../official.cpp ../../src/impl/obfuscate.cpp

