# FunctionSignature

# Overview

The FunctionSignature package is used to analyze Functions in source code and print their basic properties, such as Function Signature,
Loop information, Loads/Store information etc so that they can be used to classify these functions accordingly for DSE research.

# Installation

First we need to install all necessary tools. (LLVM-3.8 and FunctionSignature Analysis passes)


    ./bootsrap_3.8.0


The bootstrap_3.8.0.sh script downloads and builds LLVM-3.8, which is needed to compile and load the FunctionSignature passes. 

If, for any reason, you move/rename LLVM8 source tree, then you have to modify the
"LLVM_BUILD" Paths in the Makefile and the run_pass.sh script, that invokes the FunctionSignature passes, inside the 
directory for each benchmark accordingly. 

LLVM8 can then be recompiled using make and a new Shared Object (SO) should be created in order to load the FunctionSignature passes.

    cd "path/to/llvm/build" && make


# Possible errors at compile time

libsanitizer doesn't build against latest glibc anymore, see https://gcc.gnu.org/bugzilla/show\_bug.cgi?id=81066 for details.
One of the changes is that stack\_t changed from typedef struct sigaltstack { ... } stack\_t; to typedef struct { ... } stack\_t; for conformance reasons.
And the other change is that the glibc internal \_\_need\_res\_state macro is now ignored.
Follow the instructions at https://reviews.llvm.org/D35246 to fix the error. 

# Usage

For testing FunctionSignature, aes is used by Machsuite.

    cd aes

### 1) Collect dynamic profiling information and generate the annotated  Intermediate Representation (IR) files.

We make sure that the LLVM lines in "Makefile_Profile" point to the path of the LLVM-3.8 build and lib directory:    

    BIN_DIR_LLVM=path/to/llvm/build/bin
    LIB_DIR_LLVM=path/to/llvm/build/lib

Then we run the instrumented binary with the appropriate input parameters and generate the annotated IR files using
the profiling information.    

    make profile

### 2) Identification of Functions, Analysis and extract their properties.   

We make sure that the LLVM paths in "run_pass.sh" point to the path of the LLVM-3.8 build and lib directory:

    BIN_DIR_LLVM=path/to/llvm/build/bin
    LIB_DIR_LLVM=path/to/llvm/build/lib

The following script invokes the FunctionSignature Analysis passes and outputs the analysis that includes all the Function properties required.
    
    ./run_pass.sh



### Clean Up. 

To delete all prof IR related files use:

    make clean_prof_data 



** Modifications are needed to comply for every benchmark. **

# Author

Georgios Zacharopoulos georgios@seas.harvard.edu Date: May, 2020
