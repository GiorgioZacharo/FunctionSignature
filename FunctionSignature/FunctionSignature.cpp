//===------------------------- FunctionSignature.cpp -------------------------===//
//
//                     The LLVM Compiler Infrastructure
// 
// This file is distributed under the Università della Svizzera italiana (USI) 
// Open Source License.
//
// Author         : Georgios Zacharopoulos 
// Date Started   : April, 2019
//
//===----------------------------------------------------------------------===//
//
// This file identifies Functions and Loops within the functions of an
// application.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/RegionIterator.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Analysis/RegionIterator.h"
#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/Analysis/BlockFrequencyInfoImpl.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Transforms/Utils/Local.h"
#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include "llvm/IR/CFG.h"
#include "../Identify.h" // Common Header file for all RegionSeeker Passes.
#include "FunctionSignature.h"

#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugInfo.h"

#define DEBUG_TYPE "FunctionSignature"

using namespace llvm;

namespace {

  struct FunctionSignature : public FunctionPass {
    static char ID; // Pass Identification, replacement for typeid

    std::vector<Loop *> Loops_list; // Global Loop List
    std::vector<Function *> Functions_list; // Global Loop List
    std::vector<unsigned int> Functions_instr_list; // Global Loop List

    FunctionSignature() : FunctionPass(ID) {}

    // Function Identifier
    //
    bool runOnFunction(Function &F) override {

      LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
      ScalarEvolution &SE = getAnalysis<ScalarEvolutionWrapperPass>().getSE();
      std::string Function_Name = F.getName();

      Loops_list.clear(); // Clear the Loops List

      if (find_function(Functions_list, &F) == -1) 
        Functions_list.push_back(&F);

      gatherNumberOfInstructionsOfFunction(&F);
      int FuncFreq = getEntryCount(&F);

      errs() << "\n\n" <<"F[name:" <<Function_Name << "; call_freq:" << FuncFreq <<
       "; n_of_instructions:" <<  
        Functions_instr_list[ find_function(Functions_list, &F)] << "] {\n";
      // errs() << "   **********************************************" << '\n';

      getInputFunction(&F);
      getLoadsStoresLoopsOfFunction(&F, LI, SE);
      errs() << " }" << '\n';


      return false;
    }



    void gatherNumberOfInstructionsOfFunction(Function *F) {

      unsigned int NumberOfLLVMInstructions=0;

      for(Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB)
          // Iterate inside the basic block.
        for(BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE; ++BI)
          NumberOfLLVMInstructions++;

      Functions_instr_list.push_back(NumberOfLLVMInstructions);   
    }



    //
    void getCallInstrOfBB (BasicBlock *BB) {

      // Iterate inside the basic block.
      for(BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE; ++BI) {

             // Load Info
        if(CallInst *CI = dyn_cast<CallInst>(&*BI)) {

          StringRef CallName = CI->getCalledFunction()->getName();
          
          if (CallName == "llvm.dbg.value" || CallName == "llvm.lifetime.start" || CallName == "llvm.lifetime.start" ||
              CallName == "llvm.lifetime.end")
            continue;

          errs() 
          // <<  *CI  <<"\n"
          // << "OP1: " << *CI->getOperand(0) <<"\n"
          // << "OP2 " << *CI->getOperand(1) <<"\n"
          << "\t\tC[name: " << CI->getCalledFunction()->getName() 
          <<"; n_of_instructions:" <<  Functions_instr_list[ find_function(Functions_list, CI->getCalledFunction())] 
          << "]\n";
        }
      }
    }

    // Get Loads and Stores of a BB 
    //
    //
    void getLoadsandStoresOfBB (BasicBlock *BB) {


      // Iterate inside the basic block.
      for(BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE; ++BI) {

        // Load Info
        if(LoadInst *Load = dyn_cast<LoadInst>(&*BI)) {
          llvm::Type *LoadType = Load->getType();
          errs() << "\t\tR[addr:" << Load <<"; name:" << Load->getOperand(0)->getName() << "; offset:" << "NA;]" << "\n";
        }

        // Store Info
        if(StoreInst *Store = dyn_cast<StoreInst>(&*BI)) {

          llvm::Type *StoreOperandZeroType = Store->getOperand(0)->getType();
          llvm::Type *StoreOperandOneType = Store->getOperand(1)->getType();

          // if (StoreOperandZeroType->isIntegerTy())
          if (AllocaInst *alloca = dyn_cast<AllocaInst>(Store->getOperand(1))) 
            continue;
          
          else 
            errs() << "\t\tW[addr:" << Store <<"; name:" << Store->getOperand(1)->getName() << "; offset:" << "NA;]"  <<"\n";
        }
      } // End of BB For  


    }
    

    // Loops Identifier of a given function. (if any loops)
    //
    void getLoadsStoresLoopsOfFunction (Function *F, LoopInfo &LI, ScalarEvolution &SE ) {

      unsigned int NumberOfBBInstructions = 0 ; // NumberOfInstructions = 0,

      for(Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {

        BasicBlock *CurrentBlock = &*BB;
        unsigned int NumberOfBBInstructions = 0 ; // NumberOfInstructions = 0,

        for(BasicBlock::iterator BI = CurrentBlock->begin(), BE = CurrentBlock->end(); BI != BE; ++BI)
              NumberOfBBInstructions++;
          
          errs() << "\n\tBB[name:" << CurrentBlock->getName() 
            <<"; n_of_instructions:" <<  NumberOfBBInstructions << "]\n";

        
        // If-Else Analsyis (Branch Instructions Analysis)
        //           
        for(BasicBlock::iterator BI = CurrentBlock->begin(), BE = CurrentBlock->end(); BI != BE; ++BI)    
          if (BranchInst *BRI = dyn_cast<BranchInst>(&*BI)){
        

            if (BRI->isConditional()) { 
              unsigned int NumOfSuccessors = BRI->getNumSuccessors();

              for (unsigned int i=0; i< NumOfSuccessors; i++)
                errs() << "\n\t  BranchInst (IF=0,ELSE=1): " << i << "\tSuccessor BB: " << BRI->getSuccessor(i)->getName();
            }

          }

        

        // Iterate inside the Loop.
        if (Loop *L = LI.getLoopFor(CurrentBlock)) {
           if (find_loop(Loops_list, L) == -1) { // If Loop not in our list
              Loops_list.push_back(L);
              int LoopCarriedDeps = getLoopCarriedDependencies(CurrentBlock);

              if (const SCEV *ScEv = SE.getBackedgeTakenCount(L) ) {

                //errs() << "\n\n I am here 5\n "; 
                ConstantRange Range = SE.getSignedRange(ScEv);
                int stride = 0;

                //errs() << "\n\n I am here 6\n "; 


                if (SE.getSmallConstantTripCount(L))
                  stride = Range.getUpper().getLimitedValue() / SE.getSmallConstantTripCount(L);

                 //errs() << "\n\n I am here 7\n "; 

                errs() << "\n\t  L[name:" << CurrentBlock->getName() << "; depth:" << L->getLoopDepth()
                  << "; iterations:" << SE.getSmallConstantTripCount(L) 
                  << "; stride:" << stride 
                  << "; lcds:" << LoopCarriedDeps               
                  <<"; n_of_instructions:" <<  NumberOfBBInstructions
                  << "] {\n";

                 getLoadsandStoresOfBB(CurrentBlock);
                 getCallInstrOfBB(CurrentBlock);

                // errs() << "      Signed Range of Backedge Taken Count        : " << SE.getSignedRange(ScEv) << '\n';      
                // errs() << "      Range of Backedge Taken Count is            : " << Range.getUpper() - Range.getLower() << '\n';
                // errs() << "      Upper Range of Backedge Taken Count         : " << Range.getUpper()<< '\n';
                // errs() << "      Loop disposition of Backedge Taken Count is : " << SE.getLoopDisposition(ScEv, L) << "\n\n\n";
                errs() <<  "\t  }\n";
            }
          
          }
        }

        else { // No loops
          // for(BasicBlock::iterator BI = CurrentBlock->begin(), BE = CurrentBlock->end(); BI != BE; ++BI)
          //   NumberOfInstructions++;

           getLoadsandStoresOfBB(CurrentBlock);
            getCallInstrOfBB(CurrentBlock);
        }
      } // End of for
      // errs() << "   ----------------------------------------------" << '\n';
      // errs() << " Number of Instructions                  : " << NumberOfAllInstructions << "\n";
      // errs() << " Number of Instructions (Loops Excluded) : " << NumberOfInstructions << "\n";
      // errs() << "   ----------------------------------------------" << '\n';
    }



    // Metadata Information

  void getFunctionSignature(Function *F) {



    if (F->hasMetadata()) {

      MDNode *node = F->getMetadata("dbg");

      // errs() << node->getMetadataID() << "\n";
      llvm::DISubprogram *SP = F->getSubprogram();
      unsigned scopeline = SP->getScopeLine();
      unsigned line = SP->getLine();

       llvm::DIScope *Scope = dyn_cast<DIScope>(SP->getScope());

      errs() << "file: " << Scope->getDirectory()  // << "\n";
             << "/" << Scope->getFilename() << ";";

      errs() << " line_number: " << line << ";]";

    }

  }


  // START 
  // IMPORT FROM ACCELSEEKER FUNCTIONS
  //
  //
  bool structNameIsValid(llvm::Type *type) {

    if (type->getStructName() == "struct._IO_marker")
      return 0;
    if (type->getStructName() == "struct._IO_FILE")
      return 0;


    return 1;
  }

  // Gather the data of the Array type.
  //
  long int getTypeArrayData(llvm::Type *type) {

    long int array_data=0;
    int TotalNumberOfArrayElements = 1;

    while (type->isArrayTy()) {

      llvm::Type *array_type    = type->getArrayElementType();
      int NumberOfArrayElements     = type->getArrayNumElements();
      int SizeOfElement           = array_type->getPrimitiveSizeInBits();

     // errs() << "\n\t Array " << *array_type << " "  << NumberOfArrayElements<< " " << SizeOfElement  << " \n ";
      errs() << "\t A[name:" 
        // << *type
        // << *array_type
         // << array_type
         //     << &type
        << " NA" 
        <<"; type:" << *array_type 
        << "; n_bit:" << SizeOfElement << "; size:"  << NumberOfArrayElements << ";];" ;

      TotalNumberOfArrayElements *= NumberOfArrayElements;

      if (SizeOfElement) {
        array_data = TotalNumberOfArrayElements * SizeOfElement;
        return array_data ;
      }
      else
        type = array_type;
    }
    return array_data;  
  }

  long int getTypeData(llvm::Type *type){

    long int arg_data =0;

    if ( type->isPointerTy()){
       errs() << "*";


      llvm::Type *Pointer_Type = type->getPointerElementType();
      arg_data+=getTypeData(Pointer_Type);
    }

    // Struct Case
    else if ( type->isStructTy()) {

      long int struct_data=0;
      unsigned int NumberOfElements = type->getStructNumElements();

      errs() << " S["  << type->getStructName() << ";";

      StructType *struct_type = dyn_cast<StructType>(&*type);
      int i=0;

      for  (llvm::StructType::element_iterator  EI= struct_type->element_begin(), EE = struct_type->element_end(); EI != EE; ++EI, i++){

  
        llvm::Type *element_type = type->getStructElementType(i);

        errs() << "\n\t\taddr:" << EI << ";"; 

        if (structNameIsValid(type))
          struct_data +=  getTypeData(element_type);


      }


      errs() << "];";
  
      arg_data = struct_data;
      //return arg_data;    
    }

    // Scalar Case
    else if ( type->getPrimitiveSizeInBits()) {
      //errs() << "\n\t Primitive Size  " <<  type->getPrimitiveSizeInBits()  << " \n ";
      arg_data = type->getPrimitiveSizeInBits();
      errs() << "i" << arg_data;
      //return arg_data;

    }
 
    // Vector Case
    else if ( type->isVectorTy()) {
      //errs() << "\n\t Vector  " <<  type->getPrimitiveSizeInBits()  << " \n ";
      arg_data = type->getPrimitiveSizeInBits();
      //return arg_data;
    }


    // Array Case
    else if(type->isArrayTy()) {
      arg_data = getTypeArrayData(type);
      //errs() << "\n\t Array Data " << arg_data << " \n ";
      //return arg_data;
    }

    return arg_data;
  }

    // Input from parameter List.
    //
    //
    long int getInputFunction(Function *F) {
      long  int InputData = 0; // Bits
      long int InputDataBytes = 0; // Bytes

      int arg_index=0;

      Function::ArgumentListType & Arg_List = F->getArgumentList();

      for (Function::arg_iterator AB = Arg_List.begin(), AE = Arg_List.end(); AB != AE; ++AB){

        llvm::Argument *Arg = &*AB;
        llvm::Type *Arg_Type = Arg->getType();


         errs() << "\t P[addr:" << Arg <<"; name:" << AB->getName() << "; type:";

        long int InputDataOfArg = getTypeData(Arg_Type);
        errs() << " n_bit:" << InputDataOfArg << "; size:";
        //<< "NA"
        getFunctionSignature(F);
        errs() << "]\n";

        InputData += InputDataOfArg;
        arg_index++;

       }

       // errs() << "\n\n Total Input Data Bits :  " << InputData << " \n ";
       InputDataBytes = InputData/8; 
       // errs() << "\n\n Total Input Data Bytes :  " << InputDataBytes << " \n ";

      return InputDataBytes;
    }

    // Gather Stores and Loads from Functions
    //
    //
    void gatherLoadsandStores(Function *F) {
      long unsigned int NumberOfInstructions = 0;

      // BlockFrequencyInfo *BFI = &getAnalysis<BlockFrequencyInfoWrapperPass>().getBFI(); 


      for(Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
        BasicBlock *CurrentBlock = &*BB;


        errs() << "BB " << CurrentBlock->getName() << "\n";

        // Iterate inside the basic block.
        for(BasicBlock::iterator BI = CurrentBlock->begin(), BE = CurrentBlock->end(); BI != BE; ++BI) {

          // Load Info
          if(LoadInst *Load = dyn_cast<LoadInst>(&*BI)) {

            llvm::Type *LoadType = Load->getType();
            // llvm::Type *Ptr_LoadType = NULL;

            if ( LoadType->isPointerTy()) {
              llvm::Type *Ptr_LoadType = LoadType->getPointerElementType();

               if (Ptr_LoadType->isStructTy() || Ptr_LoadType->isArrayTy() || Ptr_LoadType->isVectorTy()) {
                  errs() << "\tLD\t" << *Load << "\t"  << *Load->getType() << "\t\t" << "\n";
                  getTypeData(LoadType);
                }
            }
          }

          // Store Info
          if(StoreInst *Store = dyn_cast<StoreInst>(&*BI)) {

            // errs() << "Store\n"; 
            errs() << "\tST\t" << *Store << "\t"  << *Store->getType() << "\t\t" << "\n";

            llvm::Type *StoreType = Store->getType();
            // llvm::Type *Ptr_StoreType = NULL;

            if ( StoreType->isPointerTy()) {
              llvm::Type *Ptr_StoreType = StoreType->getPointerElementType();

               if (Ptr_StoreType->isStructTy() || Ptr_StoreType->isArrayTy() || Ptr_StoreType->isVectorTy()) {
                  errs() << "\t" << *Store << "\t"  << *Store->getType() << "\t\t" << "\n";
                  getTypeData(StoreType);
                }
            }
          }

          NumberOfInstructions++;
        } // End of BB For
      } // End of Function For
      errs() << "   ----------------------------------------------" << '\n';
      errs() << " Number of Instructions : " << NumberOfInstructions << "\n";
      errs() << "   ----------------------------------------------" << '\n';
    }

    virtual void getAnalysisUsage(AnalysisUsage& AU) const override {
              
        AU.addRequired<LoopInfoWrapperPass>();
        AU.addRequiredTransitive<ScalarEvolutionWrapperPass>();
        AU.addRequired<DependenceAnalysis>();    
        // AU.addRequired<BlockFrequencyInfoWrapperPass>();
        AU.setPreservesAll();
    } 
  };
}

char FunctionSignature::ID = 0;
static RegisterPass<FunctionSignature> X("FunctionSignature", "Identify Loops within Functions");
