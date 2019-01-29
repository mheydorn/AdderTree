/*
 * AdderTreeBalancer.cpp
 *
 *  Created on: Feb 11, 2017
 *      Author: jgoeders
 */

#include "AdderTreeBalancer.h"

#include "llvm/Support/Debug.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <queue>          // std::queue
#include <iostream>          // std::queue


using namespace llvm;
namespace legup {
    std::queue<Value*> values; // empty queue with list as underlying container

    char AdderTreeBalancer::ID = 0;

    static RegisterPass<AdderTreeBalancer> X("legup-adderTreeBalancer",
                                            "Adder Tree Balancer Pass");

    AdderTreeBalancer::AdderTreeBalancer() : BasicBlockPass(ID) {
        // TODO Auto-generated constructor stub
    }

    AdderTreeBalancer::~AdderTreeBalancer() {
        // TODO Auto-generated destructor stub
    }

    void createValueList(Instruction* currentInstruction){

        //Start at the root node and gather up all the values
        for (int j = 0; j < 2; j++){
            if (Instruction * operand = dyn_cast<Instruction>(currentInstruction->getOperand(j))){
                //Is an instruction
                if (operand->getOpcode() == Instruction::Add ){
                    createValueList(operand);
                }else{
                    values.push(operand);
                } 
            }else{
                //Not an instruction
                break;
            }
        }
    }
    bool AdderTreeBalancer::runOnBasicBlock(BasicBlock &BB) {
        dbgs() << "**********************Start Basic Block**********************\n";
        dbgs() << BB << "\n";

        //Find root node
        Instruction * root = NULL;
        //for instruction i in BB:
        for(auto i = BB.begin(); i != BB.end(); i++){
            //if i is binary:
            if (BinaryOperator * bo = dyn_cast<BinaryOperator>(i)){
                //if i is an add
                if (bo->getOpcode() == Instruction::Add){
                    //check if i has no users which are binary adds
                    bool hasBinaryAddUsers = false;
                    for(auto user :bo->users()){ 
                        if (isa<BinaryOperator>(user)){
                            if (auto I = dyn_cast<Instruction>(user)){
                                if (I->getOpcode() == Instruction::Add){
                                    hasBinaryAddUsers = true;
                                    break;
                                }
                            }
                        }
                    }
                    if(!hasBinaryAddUsers){

                        //if both operands for i are add instructions:
                        bool bothOperandsAreLoadOrAdd = true;
                        for (int j = 0; j < 2; j++){
                            if (Instruction * operand = dyn_cast<Instruction>(bo->getOperand(j))){
                                //Is an instruction
                                if (operand->getOpcode() != Instruction::Add && operand->getOpcode() != Instruction::Load){
                                    //Not a laod or add instruction
                                    bothOperandsAreLoadOrAdd = false;
                                    break;
                                } 
                            }else{
                                //Not an instruction
                                bothOperandsAreLoadOrAdd = false;
                                break;
                            }
                        }
                        if (bothOperandsAreLoadOrAdd){
                            root = i;
                            
                        }
                    }
                }
            }      
        }

        //Create list of inputs
        if (root == NULL){
            dbgs() << "root node is null\n";
            dbgs() << "**********************End Basic Block**********************\n";
            return false;


        }

        dbgs() << "root node " << *root << "\n";

        createValueList(root);
        
        dbgs() << "Content of value queue:\n";
        std::queue<Value*> tempqueue(values);
        while(tempqueue.size() > 0){
            dbgs() << *(tempqueue.front()) << "\n";
            tempqueue.pop();
        }

        int instNum = 0;
        while(values.size() != 2){
            Value * v1 = values.front();
            values.pop();
            Value * v2 = values.front();
            values.pop();
            dbgs() << *v1 << *v2 << "\n";
            char name [40];
            int ignore = sprintf(name, "instruction %d", instNum);
            instNum++;
            BinaryOperator * newAdd = BinaryOperator::Create(BinaryOperator::Add, v1, v2, name, (Instruction*) NULL);
            BB.getInstList().insert(root, newAdd);
            values.push(newAdd);
        }

        Value * v1 = values.front();
        values.pop();
        Value * v2 = values.front();

        Instruction * root2 = dyn_cast<Instruction>(root);
        char name [40];
        int ignore = sprintf(name, "instruction %d", instNum);
        instNum++;

        BinaryOperator * newRoot = BinaryOperator::Create(BinaryOperator::Add, v1, v2,name, (Instruction*) NULL);
        Instruction * newRoot2 = dyn_cast<Instruction>(newRoot);

        ReplaceInstWithInst(root2, newRoot2);
        dbgs() << "***********************End Basic Block**********************\n";

        return true;
    }
}
