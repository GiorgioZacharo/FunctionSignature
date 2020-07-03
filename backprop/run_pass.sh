############### This is the Function/Loop Analysis on the whole app ##############
#
#
#
#    Georgios Zacharopoulos <georgios.zacharopoulos@usi.ch>
#    Date: April, 2019
#    Universita' della Svizzera italiana (USI Lugano)
############################################################################################### 

#!/bin/bash

#BENCH=aes
BENCH=backprop

# Paths to LLVM Latest version (3.8) (Edit the Path)
BIN_DIR_LLVM=~giorgio/llvm_new/build/bin
LIB_DIR_LLVM=~giorgio/llvm_new/build/lib
#BIN_DIR_LLVM=~/FunctionSignature/llvm-3.8.0/build/bin
#LIB_DIR_LLVM=~/FunctionSignature/llvm-3.8.0/build/lib

#BIN_DIR_CLANG=~/llvm-project/build/bin/
#cd src_$BENCH

#echo "--> 1. Create LLVM-IR from C"
#for i in `ls src_$BENCH/*.c`; do  $BIN_DIR_LLVM/clang -O1 -g -S -emit-llvm $i -o $i.ir; done

rm *.app.ir
mkdir IR; mv  *.ir IR/.

echo "--> 2. Link the LLVM-IR to a single ir file"
#$BIN_DIR_LLVM/llvm-link -S  *.ir -o $BENCH.app.ir
$BIN_DIR_LLVM/llvm-link -S  IR/backprop.ir -o $BENCH.app.ir

echo "--> 4. Load the FunctionSignature Pass"
$BIN_DIR_LLVM/opt -load $LIB_DIR_LLVM/FunctionSignature.so -mem2reg  -FunctionSignature -stats    > /dev/null  $BENCH.app.ir
#$BIN_DIR_LLVM/opt -load $LIB_DIR_LLVM/FunctionSignature.so -FunctionSignature -stats    > /dev/null  $BENCH.ir
