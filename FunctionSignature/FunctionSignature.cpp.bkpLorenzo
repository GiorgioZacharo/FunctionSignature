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
#include "llvm/IR/Metadata.h"

#define DEBUG_TYPE "FunctionSignature"

using namespace llvm;

namespace {

  struct FunctionSignature : public FunctionPass {
    static char ID; // Pass Identification, replacement for typeid

    std::vector<Instruction *> Inst_list;
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

      int *indentLevel;
      *indentLevel = 0;
      
      errs() << "\n\n" << "F["
	      << "name:" << Function_Name << ";" 
	      << "call_freq:" << FuncFreq << ";"
       	      << "n_of_instructions:" <<  Functions_instr_list[ find_function(Functions_list, &F)] << ";" 
	      << "]{\n";
      (*indentLevel)++;
      
      getFunctionParameters(&F, indentLevel);
      
      for(Function::iterator BB = F.begin(), E = F.end(); BB != E; ++BB) {
          BasicBlock *CurrentBlock = &*BB;
      	  analyseBasicBlock(CurrentBlock, LI, SE, indentLevel);
      }
      errs() << "}" << '\n';
      (*indentLevel)--;
      return false;
    }

   int find_inst(std::vector<Instruction *> list, Instruction *inst) {
       for (unsigned i = 0; i < list.size(); i++)
	       if (list[i] == inst)
		       return i;
       return -1;
   }

   void analyseBasicBlock(BasicBlock *CurrentBlock, LoopInfo &LI, ScalarEvolution &SE, int * indentLevel){
	//int NumberOfBBInstructions = 0;
	if (Loop *L = LI.getLoopFor(CurrentBlock)) {
	   if (find_loop(Loops_list, L) == -1) { // If Loop not in our list
    		   Loops_list.push_back(L);	
  		   
		   int LoopCarriedDeps = getLoopCarriedDependencies(CurrentBlock);
		   int stride = 0;
		   if (const SCEV *ScEv = SE.getBackedgeTakenCount(L) ) {
		       ConstantRange Range = SE.getSignedRange(ScEv);
		       if (SE.getSmallConstantTripCount(L)){
		           stride = Range.getUpper().getLimitedValue() / SE.getSmallConstantTripCount(L);
		       }
		   }

		   //for(BasicBlock::iterator BI = CurrentBlock->begin(), BE = CurrentBlock->end(); BI != BE; ++BI){
    		   //   NumberOfBBInstructions++;
	   	   //}
		   errs() << printIndentation(indentLevel)
			  << "L["
			  << "name:" << CurrentBlock->getName() << ";"
			  << "depth:" << L->getLoopDepth() << ";"
           		  << "iterations:" << SE.getSmallConstantTripCount(L) << ";"
           		  << "stride:" << stride << ";"
			  << "lcds:" << LoopCarriedDeps << ";"
			  << "]{\n";

		   (*indentLevel)++;

                   for(BasicBlock *BB : L->getBlocks()){
	 		   analyseBasicBlock(BB, LI, SE, indentLevel);	
		   }
		   (*indentLevel)--;
                   errs() << printIndentation(indentLevel) << "}" << '\n';
	   }	
       }
       visitBasicBlock(CurrentBlock, indentLevel);
    }

    void visitBasicBlock(BasicBlock *BB, int *indentLevel){
        for(BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE; ++BI){
            Instruction *CurrentInstruction = &*BI;
	    if (find_inst(Inst_list, CurrentInstruction) == -1) { // If instruction not in our list
		Inst_list.push_back(CurrentInstruction);

                if(isa<CallInst>(CurrentInstruction)){
		    analyseCallInstruction(CurrentInstruction, indentLevel);
	        }
	        if (isa<LoadInst>(CurrentInstruction)){
		    analyseReadInstruction(CurrentInstruction, indentLevel);
                }
                if (isa<StoreInst>(CurrentInstruction)){
		    analyseWriteInstruction(CurrentInstruction, indentLevel);
                }
	        if (isa<AllocaInst>(CurrentInstruction)){
		    analyseAllocationInstruction(CurrentInstruction, indentLevel);
                }
	    }
	}
    }

    std::string printIndentation(int *indentationLevel){
	std::string tabs = "";
	for(int i=0; i<*indentationLevel; i++){
            tabs += "\t";
	}
	return tabs;
    }

    void analyseAllocationInstruction(Instruction *CurrentInstruction, int *indentLevel){
        AllocaInst *allocInst = cast<AllocaInst>(CurrentInstruction);
        if(allocInst->isArrayAllocation()){
            //errs() << "A\n";
	    Type* t = allocInst->getType();
	    allocArrayData(allocInst, t, indentLevel);
        }
        if(allocInst->isStaticAlloca()){
            Type* t = allocInst->getType();
	    allocArrayData(allocInst, t, indentLevel);
            //if(t->isPointerTy()){
            //    Type *pT = t->getPointerElementType();
            //    bool isArray = false;
            //    if(pT->isArrayTy()){ //Change it in a while to explore all the dimension of the array
            //        isArray = true;
            //    }
            //    if (isArray==true){
            //        errs() << "A\n";
        	//}
	    //}
	}
    }

    void analyseWriteInstruction(Instruction *CurrentInstruction, int *indentLevel){
	StoreInst *s = cast<StoreInst>(CurrentInstruction);
        Value* v = s->getPointerOperand();
        if(isa<GEPOperator>(v)){
          errs() << printIndentation(indentLevel)
		 << "W["
                 << "addr:" << s << ";"
                 << "name:" << s->getOperand(1)->getName() << ";"
                 << "offset:" << "NA;"
                 << "]" << "\n";   
	} 
    }

    void analyseReadInstruction(Instruction *CurrentInstruction, int *indentLevel){
        LoadInst *l = cast<LoadInst>(CurrentInstruction);
        Value* v = l->getPointerOperand();
        if(isa<GEPOperator>(v)){
          errs() << printIndentation(indentLevel)
		 << "R[" 
		 << "addr:" << l << ";" 
		 << "name:" << l->getOperand(0)->getName() << ";" 
		 << "offset:" << "NA;"
		 << "]" << "\n";
        }
    }

    void analyseCallInstruction(Instruction *CurrentInstruction, int *indentLevel){
        StringRef CallName = cast<CallInst>(CurrentInstruction)->getCalledFunction()->getName();
	if (!(CallName == "llvm.dbg.value" || CallName == "llvm.lifetime.start" || CallName == "llvm.lifetime.start" || CallName == "llvm.lifetime.end" 
				|| CallName == "llvm.dbg.declare")){
	    if (CallName.find("llvm.memset") != std::string::npos){
		//StoreInst *s = cast<StoreInst>(CurrentInstruction);
		//Value *value = CurrentInstruction->getValueID();
		//errs() << "Store\n";
		    errs() << printIndentation(indentLevel)
                       << "W["
                       << "addr:" << ";"
                       << "name:" << ";"
                       << "offset:" << "NA;"
                       << "]" << "\n";
		      
	    }else{
          
                errs() << printIndentation(indentLevel)
                    << "C[" 
	            << "name:" << CallName << ";"
                    << "n_of_instructions:" <<  Functions_instr_list[ find_function(Functions_list, cast<CallInst>(CurrentInstruction)->getCalledFunction())] << ";"
                    << "]\n";
	   }
	}
    }    

    void gatherNumberOfInstructionsOfFunction(Function *F) {

      unsigned int NumberOfLLVMInstructions=0;

      for(Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB)
          // Iterate inside the basic block.
        for(BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE; ++BI)
          NumberOfLLVMInstructions++;

      Functions_instr_list.push_back(NumberOfLLVMInstructions);   
    }

//    void gatherNumberOfInstructionsOfLoop(Loop *L) {
//
//      unsigned int NumberOfLLVMInstructions=0;
//
//            for(Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB){
//		    // Iterate inside the basic block.
//	    	for(BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE; ++BI){
//	                NumberOfLLVMInstructions++;
//	    	}
//		Functions_instr_list.push_back(NumberOfLLVMInstructions);
//	    }
//    }

  //void getFunctionSignature(Function *F) {
  std::string getFunctionSignatureMetadata(Function *F) {
	  std::string metadata = "";
	  if (F->hasMetadata()) {

      		MDNode *node = F->getMetadata("dbg");

      		llvm::DISubprogram *SP = F->getSubprogram();
      		unsigned scopeline = SP->getScopeLine();
      		unsigned line = SP->getLine();
      		llvm::DIScope *Scope = dyn_cast<DIScope>(SP->getScope());

      		metadata += "metadata[file:";
	       	metadata += Scope->getDirectory();
             	metadata += "/";
	        metadata += Scope->getFilename();
	        metadata += ";";
	        metadata += "line_number:";
	        metadata += std::to_string(line);
		metadata += ";]";
    	}
	return metadata;
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


    void allocArrayData(AllocaInst *alloc, Type *t, int * indentLevel) {

        long int array_data=0;
	int TotalNumberOfArrayElements = 1;
	if(t->isPointerTy()){
	    Type *pT = t->getPointerElementType();
	    bool isArray = false;
	    if(pT->isArrayTy()){ //Change it in a while to explore all the dimension of the array
       		    llvm::Type *array_type    = t->getArrayElementType();
		    errs() << printIndentation(indentLevel) 
		       << "A[name:" << alloc->getName() << ";"
		       << "type:" << *(alloc->getAllocatedType()) << ";"
		       << "n_bit:" << array_type->getPrimitiveSizeInBits() << ";"
		       //<< "]\n";
		       << "size:" << pT->getArrayNumElements() << ";];\n";
	    }
	}
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
        << "NA" 
        <<";type:" << *array_type 
        << ";n_bit:" << SizeOfElement << ";size:"  << NumberOfArrayElements << ";];" ;

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

    // Pointer case
    if ( type->isPointerTy()){
       errs() << "*";

      llvm::Type *Pointer_Type = type->getPointerElementType();
      arg_data+=getTypeData(Pointer_Type);
    }

    // Struct Case
    else if ( type->isStructTy()) {

      long int struct_data=0;
      unsigned int NumberOfElements = type->getStructNumElements();

      errs() << "S[name:" << type->getStructName() << ";";

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
    }

    // Scalar Case
    else if ( type->getPrimitiveSizeInBits()) {
      //errs() << "\n\t Primitive Size  " <<  type->getPrimitiveSizeInBits()  << " \n ";
      arg_data = type->getPrimitiveSizeInBits();
      errs() << arg_data;
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
    void getFunctionParameters(Function *F, int * indentLevel){

      Function::ArgumentListType & Arg_List = F->getArgumentList();

      for (Function::arg_iterator AB = Arg_List.begin(), AE = Arg_List.end(); AB != AE; ++AB){

        llvm::Argument *Arg = &*AB;
        llvm::Type *Arg_Type = Arg->getType();

         errs() << printIndentation(indentLevel)
		<<  "P[" 
		<< "addr:" << Arg << ";"
          	<< "name:" << AB->getName() << ";"
	        << "type:";

        long int InputDataOfArg = getTypeData(Arg_Type);
        errs() << "n_bit:" << InputDataOfArg << ";" 
		<< "size:" << getFunctionSignatureMetadata(F) << ";"
        	<< "]\n";

       }

    }

    virtual void getAnalysisUsage(AnalysisUsage& AU) const override {
              
        AU.addRequired<LoopInfoWrapperPass>();
        AU.addRequiredTransitive<ScalarEvolutionWrapperPass>();
        // AU.addRequired<RegionInfoPass>();
        AU.addRequired<DependenceAnalysis>();
        // AU.addRequiredTransitive<RegionInfoPass>();      
        // AU.addRequired<BlockFrequencyInfoWrapperPass>();
        AU.setPreservesAll();
    } 
  };
}

char FunctionSignature::ID = 0;
static RegisterPass<FunctionSignature> X("FunctionSignature", "Identify Loops within Functions");
