//===------------------------- FunctionSignature.h -------------------------===//
//
//                     The LLVM Compiler Infrastructure
// 
// This file is distributed under the Università della Svizzera italiana (USI) 
// Open Source License.
//
// Author         : Georgios Zacharopoulos 
// Date Started   : June, 2018
//
//===----------------------------------------------------------------------===//
//
// This file identifies Functions and Loops within the functions of an
// application.
//
//===----------------------------------------------------------------------===//


  int find_function(std::vector<Function *> list, Function *F) {

    for (unsigned i = 0; i < list.size(); i++)
      if (list[i] == F)
        return i;
    
    return -1;
  }


    int getEntryCount(Function *F) {

      int entry_freq = 0;

      if (F->hasMetadata()) {

        MDNode *node = F->getMetadata("prof");

        if (MDString::classof(node->getOperand(0))) {
          auto mds = cast<MDString>(node->getOperand(0));
          std::string metadata_str = mds->getString();

          if (metadata_str == "function_entry_count"){
            if (ConstantInt *CI = mdconst::dyn_extract<ConstantInt>(node->getOperand(1))) {
              entry_freq = CI->getSExtValue();
              //errs() <<" Func_Freq " << entry_freq << " "; //  Turn it back on mayne.
            }              

          }
        }
      }

      return entry_freq;
    }


bool isInductionVariable(Value *DependencyVar) {

  if (Operator *DependencyInstr = dyn_cast<Operator>(DependencyVar)) {

    if (DependencyInstr->getOpcode() == Instruction::Add) {

      if (ConstantInt *constant = dyn_cast<ConstantInt>(DependencyInstr->getOperand(1)) )
        if (constant->getSExtValue() == 1 || constant->getSExtValue() == -1 )
          return true;
    }
  }

  return false; 
}

  unsigned int getLoopCarriedDependencies(BasicBlock *BB) {

  unsigned int LoopCarriedDep = 0;
  bool IndVariableFound = false;

  //DependenceAnalysis *DA = &getAnalysis<DependenceAnalysis>();

  for(BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE; ++BI) {

    if (PHINode *PN  = dyn_cast<PHINode>(&*BI)) {

      for (unsigned int i =0; i < PN->getNumIncomingValues(); i++) {
        if (PN->getIncomingBlock(i) == BB) {
          LoopCarriedDep++;
          //errs() << "     Phi : " << *PN << "\n";
          Value *DependencyVar = PN->getIncomingValueForBlock(BB);

          if (!IndVariableFound)
            if (isInductionVariable(DependencyVar)){
              LoopCarriedDep--;
              IndVariableFound = true;
              //errs() << "     Induction Variable Found! " << "\n";
            }

          // Instruction * DepInstr = dyn_cast<Instruction>(DependencyVar);
          // std::unique_ptr<Dependence> Dep = DA->depends( BI, BI, false);

          // if  (Dep->isInput())
          //   errs() << " True Dependence! \n " ;

        }
      }
    }
  }

  return LoopCarriedDep;
}
