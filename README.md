# FunctionSignature

# Overview

The FunctionSignature package is used to analyze Functions in source code and print their basic properties, such as Function Signature,
Loop information, Loads/Store information etc so that they can be used to classify these functions accordingly for DSE research.

# Installation

First we need to install all necessary tools. (LLVM8 and FunctionSignature Analysis passes)


    ./bootsrap_3.8.0


The bootstrap.8.0.sh script downloads and builds LLVM8, which is needed to compile and load the FunctionSignature passes. 

If, for any reason, you move/rename LLVM8 source tree, then you have to modify the
"LLVM_BUILD" Paths in the Makefile and the run_pass.sh script, that invokes the FunctionSignature passes, inside the 
directory for each benchmark accordingly. 

LLVM8 can then be recompiled using make and a new Shared Object (SO) should be created in order to load the FunctionSignature passes.

    cd "path/to/llvm/build" && make


# Usage

For testing FunctionSignature, aes is used by Machsuite.

    cd aes

### 1) Collect dynamic profiling information and generate the annotated  Intermediate Representation (IR) files.

We make sure that the LLVM lines in "Makefile_Profile" point to the path of the LLVM8 build and lib directory:    

    BIN_DIR_LLVM=path/to/llvm/build/bin
    LIB_DIR_LLVM=path/to/llvm/build/lib

Then we run the instrumented binary with the appropriate input parameters and generate the annotated IR files using
the profiling information.    

    make profile

### 2) Identification of Functions, Analysis and extract their properties.   

We make sure that the LLVM_BUILD line in "run_pass.sh" points to the path of the LLVM8 build directory:

    LLVM_BUILD=path/to/llvm/build

The following script invokes the FunctionSignature Analysis passes and outputs the analysis that includes all the Function properties required.
    
    ./run_pass.sh



### Clean Up. 

To delete all prof IR related files use:

    make clean_prof_data 



** Modifications are needed to comply for every benchmark. **

# Author

Georgios Zacharopoulos georgios@seas.harvard.edu Date: May, 2020
