#include "codegen.h"

/*
L1 vs L2
get operand of branch isntruction
operand 0 of a branch instruction is the condition
op 1 %6 = false
op 2 %9 = true

OPERAND VS INSTRUCTION
%6 = load i32, ptr %3, align 4 todo
    1 operand; %6 is the instruction itself

store i32 %0, ptr %2, align 4
    2 operands; since store doesn't have a creation no LHS

is this how to check if P is a temporary variable and has a physical register
    temp var is related to llvm
    llvm has no physical registers
    when translating into machine code, we assign a temp variable a physical register
 
        check if the parameter P is a temporary var %p
        aka check if it's an instruction

        checking physical register is if i assigned something

        IF YOU HAVE IN REGISTER it's not being used from the memory
    
/*
todo
for every instruction
line adds to # then dump llvmvalueref
then newline
*/

//reg is -1 or spill then var is in memory
// are all var temp? answer: no, either funcParameter, or constant, or temporary variable

// Emit functions
void returnStatements(LLVMValueRef inst);
void loadStatements(LLVMValueRef inst);
void storeStatements(LLVMValueRef inst, LLVMValueRef func_parameter);
void callStatements(LLVMValueRef inst, LLVMValueRef func);
void brStatements(LLVMValueRef inst);
void allocaStatements(LLVMValueRef inst);
void mathStatements(LLVMValueRef inst);
void compareStatements(LLVMValueRef inst);

/* ASSEMBLY CODE */
void createBBLabels(LLVMModuleRef module);
void printDirectives(LLVMValueRef func);
void printFunctionEnd();
void getOffsetMap(LLVMValueRef func);

/* IMPORTANT VARIABLES */
unordered_map<LLVMValueRef, string> reg_map; //todo make reg_map[inst] = -1 for all inst
unordered_map<LLVMValueRef, int> offset_map;
int localMem;
LLVMValueRef funcParameter = NULL;

// instructions
unordered_map<LLVMValueRef, int> inst_index;
vector<LLVMValueRef> sorted_insts;
unordered_map<LLVMValueRef, pair<int, int>> live_range;

/* HELPER FUNCTIONS */
void compute_liveness(LLVMBasicBlockRef bb);
auto compare_instructions_func = [](LLVMValueRef inst1, LLVMValueRef inst2)
{
    return live_range[inst1].second > live_range[inst2].second;
};

// operand, opcode
unordered_set<int> no_result_operands = {LLVMBr, LLVMRet, LLVMCall, LLVMStore};
unordered_set<LLVMOpcode> math_opcodes = {LLVMAdd, LLVMSub, LLVMMul};

// basic block
unordered_map<LLVMBasicBlockRef, string> bb_labels;

void compute_liveness(LLVMBasicBlockRef bb)
{
    // Getting the index of each instruction in the basic block
    int idx = 0;
    for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); instruction != NULL; instruction = LLVMGetNextInstruction(instruction))
    {
        if (LLVMIsAAllocaInst(instruction))
            continue;                  // skip allocs
        inst_index[instruction] = idx; // start of liveness
        live_range[instruction] = make_pair(idx, -1);
        idx++;
    }

    // Getting the live range of each instruction in the basic block
    for (auto &inst_pair : inst_index)
    {
        for (LLVMValueRef instruction = LLVMGetLastInstruction(bb); instruction != NULL; instruction = LLVMGetPreviousInstruction(instruction))
        {
            for (int idx = 0; idx < LLVMGetNumOperands(instruction); idx++)
            {
                if (LLVMGetOperand(instruction, idx) == inst_pair.first)
                {
                    live_range[inst_pair.first].second = inst_index[instruction];
                }
            }
        }
    }
}

void sort_by_liveness()
{
    sorted_insts.clear();
    sort(sorted_insts.begin(), sorted_insts.end(), compare_instructions_func);
}

string get_free_register(const unordered_map<string, bool> &registerStatus)
{
    for (const auto &registerPair : registerStatus)
    {
        if (!registerPair.second)
        {
            return registerPair.first;
        }
    }
    return "";
}

// uses reg_map
LLVMValueRef find_spill()
{
    LLVMValueRef returning = NULL;

    // Loop through each instruction in sorted_insts
    for (auto &curr_instr : sorted_insts)
    {
        // Check if the instruction is in the map and its value isn't "-1"
        auto foundIterator = reg_map.find(curr_instr);
        if (foundIterator != reg_map.end() && foundIterator->second != "-1")
        {
            returning = curr_instr;
            break;
        }
    }

    return returning;
}

void allocateRegisters(LLVMModuleRef m)
{
    for (LLVMValueRef func = LLVMGetFirstFunction(m); func != NULL; func = LLVMGetNextFunction(func))
    {
        for (LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(func); block != NULL; block = LLVMGetNextBasicBlock(block))
        { // 1.1

            unordered_map<string, bool> registerStatus;
            registerStatus["ebx"] = false;
            registerStatus["ecx"] = false;
            registerStatus["edx"] = false;

            compute_liveness(block); // 1.2
            sort_by_liveness();      // 1.3

            LLVMOpcode currentOpcode = LLVMGetInstructionOpcode(LLVMGetFirstInstruction(block));

            for (LLVMValueRef Instr = LLVMGetFirstInstruction(block); Instr != NULL; Instr = LLVMGetNextInstruction(Instr))
            { // 1.4
                if (LLVMIsAAllocaInst(Instr))
                    continue; // 1.4.1

                if (no_result_operands.find(LLVMGetInstructionOpcode(Instr)) != no_result_operands.end())
                { // 1.4.2
                    for (int index = 0; index < LLVMGetNumOperands(Instr); index++)
                    {

                        LLVMValueRef currentOperand = LLVMGetOperand(Instr, index);

                        // 1.4.2.1
                        if (live_range[currentOperand].second == inst_index[Instr] && reg_map.find(currentOperand) != reg_map.end())
                            registerStatus[reg_map[currentOperand]] = false;
                    }
                }
                else
                {
                    string freeRegister = get_free_register(registerStatus);

                    // 1.4.3.1
                    if ((math_opcodes.find(currentOpcode) != math_opcodes.end()) &&
                        (reg_map.find(LLVMGetOperand(Instr, 0)) != reg_map.end()) &&
                        (live_range[LLVMGetOperand(Instr, 0)].second == inst_index[Instr]))
                    {
                        // 1.4.3.1.1
                        string newRegister = reg_map[LLVMGetOperand(Instr, 0)];
                        reg_map[Instr] = newRegister;
                        registerStatus[newRegister] = true;

                        // 1.4.3.1.2 if live range of second operand of Instr ends
                        LLVMValueRef secondOperand = LLVMGetOperand(Instr, 1);
                        if (reg_map.find(secondOperand) != reg_map.end())
                        {
                            if (live_range[secondOperand].second == inst_index[Instr]) // it has a physical register P assigned to it then add P to available set of registers.
                            {
                                registerStatus[reg_map[secondOperand]] = false;
                            }
                        }
                    }
                    else if (!freeRegister.empty()) // 1.4.3.2
                    {
                        // 1.4.3.2.1
                        reg_map[Instr] = freeRegister;

                        // 1.4.3.2.2
                        registerStatus[freeRegister] = true;

                        // 1.4.3.2.3
                        for (int index = 0; index < LLVMGetNumOperands(Instr); index++)
                        {
                            LLVMValueRef currentOperand = LLVMGetOperand(Instr, index);
                            if (reg_map.find(currentOperand) != reg_map.end())
                            {
                                if (live_range[currentOperand].second == inst_index[Instr])
                                {
                                    registerStatus[reg_map[currentOperand]] = false;
                                }
                            }
                        }
                    }
                    else // 1.4.3.3 physical register is not available
                    {
                        LLVMValueRef V = find_spill(); // 1.4.3.3.1

                        if (live_range[V].second > live_range[Instr].second)
                        {
                            reg_map[Instr] = "-1"; // 1.4.3.3.2
                        }
                        else
                        {
                            // 1.4.3.3.3
                            string R = reg_map[V];
                            reg_map[Instr] = R;
                            reg_map[V] = "-1";
                        }

                        // 1.4.3.3.4
                        // If the live range of any operand of Instr ends,
                        for (int index = 0; index < LLVMGetNumOperands(Instr); index++)
                        {
                            LLVMValueRef currentOperand = LLVMGetOperand(Instr, index);
                            if (live_range[currentOperand].second == inst_index[Instr]) // If the live range of any operand of Instr ends
                            {
                                if (reg_map.find(currentOperand) != reg_map.end()) // and it has a physical register P assigned to it,
                                {
                                    registerStatus[reg_map[currentOperand]] = false; // then add P to the available set of registers.
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void codegen(LLVMModuleRef module)
{
    createBBLabels(module);

    for (LLVMValueRef func = LLVMGetFirstFunction(module); func != NULL; func = LLVMGetNextFunction(func))
    {                          // 1
        printDirectives(func); // 2, 4-7
        getOffsetMap(func);    // 3

        // 8
        for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(func); bb != NULL; bb = LLVMGetNextBasicBlock(bb))
        {
            string bb_label = bb_labels[bb];

            // 8.1
            fprintf(stdout, "%s:\n", bb_label.c_str()); // Define label for the basic block

            // 8.2
            for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst != NULL; inst = LLVMGetNextInstruction(inst))
            {
                LLVMDumpValue(inst);
                switch (LLVMGetInstructionOpcode(inst))
                {
                case LLVMRet:
                    returnStatements(inst);
                    break;
                case LLVMLoad:
                    loadStatements(inst);
                    break;
                case LLVMStore:
                    storeStatements(inst, funcParameter);
                    break;
                case LLVMCall:
                    callStatements(inst, func);
                    break;
                case LLVMBr:
                    brStatements(inst);
                    break;
                case LLVMAlloca:
                    break;
                case LLVMAdd:
                    mathStatements(inst);
                    break;
                case LLVMSub:
                    mathStatements(inst);
                    break;
                case LLVMMul:
                    mathStatements(inst);
                    break;
                case LLVMICmp:
                    compareStatements(inst);
                    break;
                default:
                    break;
                }
            }
        }
    }
}

// CODEGEN HELPERS
void createBBLabels(LLVMModuleRef module)
{
    bb_labels.clear();

    for (LLVMValueRef func = LLVMGetFirstFunction(module); func != NULL; func = LLVMGetNextFunction(func))
    {
        int bbCount = 0;
        for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(func); bb != NULL; bb = LLVMGetNextBasicBlock(bb))
        {
            bb_labels[bb] = "BB" + to_string(bbCount);
            bbCount++;
        }
    }
}

void printDirectives(LLVMValueRef func)
{
    fprintf(stdout, ".section .text\n");
    fprintf(stdout, ".globl %s\n", LLVMGetValueName(func));
    fprintf(stdout, ".type %s, @function\n", LLVMGetValueName(func));
    fprintf(stdout, "%s:\n", LLVMGetValueName(func));
    fprintf(stdout, "\tpushl %%ebp\n");
    fprintf(stdout, "\tmovl %%esp, %%ebp\n");
    fprintf(stdout, "\tsubl $%d, %%esp\n", localMem);
    fprintf(stdout, "\tpushl %%ebx\n");
}

void printFunctionEnd()
{
    fprintf(stdout, "\tmovl %%ebp, %%esp\n");
    fprintf(stdout, "\tpopl %%ebp\n");
    fprintf(stdout, "\tleave\n");
    fprintf(stdout, "\tret\n");
}

/*
stack:
call operation (func param) = 8
return address = 4
then in the negative direction... = 0
we always make local copy of func param
move offset_map so that local copy and func param have same

ex
define i32 @0(i32 %0) { // offset_map = 8
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4 // change so that offset_map = 8
  %6 = alloca i32, align 4
  store i32 %0, ptr %5, align 4 // change so that offset_map = 8
> storing %0 in %5

at the store, we check if %0 == %0 (same memory) then we set the offsets
*/
void getOffsetMap(LLVMValueRef func)
{
    offset_map.clear();
    localMem = 4;
    if (LLVMCountParams(func) > 0) {
        funcParameter = LLVMGetFirstParam(func);
        offset_map[funcParameter] = 8; //when you do call operation, push return addr onto stack
    }

    int offset = 0;
    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(func); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
        for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst != NULL; inst = LLVMGetNextInstruction(inst)) {
            if (LLVMIsAAllocaInst(inst)) {
                localMem += 4;
                offset_map[inst] = (-1*localMem);
            }
            else if (LLVMIsAStoreInst(inst)) { //store i32 %a, ptr %b, align 4
                LLVMValueRef a = LLVMGetOperand(inst, 0); // local copy of funcParam
                LLVMValueRef b = LLVMGetOperand(inst, 1);
                if (a == funcParameter) {
                    int x = offset_map[a]; //store operation to make local copy of your parameter
                    offset_map[b] = x;
                }
                else {
                    int x = offset_map[b]; //ask why swapped here
                    offset_map[a] = x;
                }
            }
            else if (LLVMIsALoadInst(inst)) {
                LLVMValueRef a = LLVMGetOperand(inst, 0);
                int x = offset_map[a];
                offset_map[inst] = x;
            }
        }
    }
}

// done
// like (ret i32 A)
void returnStatements(LLVMValueRef inst)
{
    LLVMValueRef A = LLVMGetOperand(inst, 0);

    if (LLVMIsConstant(A)) // 1
        fprintf(stdout, "\tmovl $%lld, %%eax\n", LLVMConstIntGetSExtValue(A));

    // if A is a temporary variable and is in memory
    if (reg_map.find(A) != reg_map.end() && reg_map[A] == "-1") // 2
        fprintf(stdout, "\tmovl %d(%%ebp), %%eax\n", offset_map[A]);


    // if A is a temporary variable and has a physical register
    if (reg_map.find(A) != reg_map.end() && reg_map[A] != "-1") // 3
    {
        fprintf(stdout, "\tmovl %%%s, %%eax\n", reg_map[A].c_str());
    }

    fprintf(stdout, "\tpopl %%ebx\n"); // 4
    printFunctionEnd();                    // 5
}

// done
// like (%a = load i32, i32* %b)
// %b is the alloc instruction
// %a is the load instruction
void loadStatements(LLVMValueRef a)
{
    LLVMValueRef b = LLVMGetOperand(a, 0);

    if (reg_map.find(a) != reg_map.end() && reg_map[a] != "-1") //if %a has been assigned a physical register %exx
        fprintf(stdout, "\tmovl %d(%%ebp), %%%s\n", offset_map[b], reg_map[a].c_str());
    
}

// done
// like (store i32  A, i32* %b)
void storeStatements(LLVMValueRef store_inst, LLVMValueRef funcParameter)
{
    LLVMValueRef A = LLVMGetOperand(store_inst, 0);
    LLVMValueRef b = LLVMGetOperand(store_inst, 1);

    if (funcParameter == A) // 1 
        return;
    else if (LLVMIsConstant(A)) // 2
    {
        fprintf(stdout, "\tmovl $%lld, %d(%%ebp)\n", LLVMConstIntGetSExtValue(A), offset_map[b]);
    }
    else // 3 A is a temporary variable %a
    {
        if (reg_map[A] != "-1") //if %a has a physical register %exx assigned to it
        {
            fprintf(stdout, "\tmovl %%%s, %d(%%ebp)\n", reg_map[A].c_str(), offset_map[b]);
        }
        else
        {
            int c1 = offset_map[A];
            fprintf(stdout, "\tmovl %d(%%ebp), %%eax\n", c1);

            int c2 = offset_map[b];
            fprintf(stdout, "\tmovl %%eax, %d(%%ebp)\n", c2);
        }
    }
    return;
}

// done? but askv
// like ( %a = call type @func(P)) or (call type @func(P))
void callStatements(LLVMValueRef inst, LLVMValueRef func)
{
    fprintf(stdout, "\tpushl %%ecx\n");
    fprintf(stdout, "\tpushl %%edx\n");

    // 3, if func has a P
    if (LLVMCountParams(func) == 1)
    {
        LLVMValueRef P = LLVMGetParam(func, 0);

        if (LLVMIsConstant(P))
            fprintf(stdout, "\tpushl $%lld\n", LLVMConstIntGetSExtValue(P));
        else // if P is a temporary variable %p
        {
            if (reg_map.find(P) != reg_map.end() && reg_map[P] != "-1")
                fprintf(stdout, "\tpushl %%%s\n", reg_map[P].c_str());
            else if (reg_map.find(P) != reg_map.end()) 
            {
                int k = offset_map[P];
                fprintf(stdout, "\tpushl %d(%%ebp)\n", k);
            }
        }
    }

    fprintf(stdout, "\tcall func\n"); // 4
    
    if (LLVMCountParams(func) == 1) // 5
        fprintf(stdout, "\taddl $4, %%esp\n");

    // askv if Instr is of the form (%a = call type @func())
    if (LLVMGetNumOperands(inst) == 1)
    {
        LLVMValueRef a = LLVMGetOperand(inst, 0);
        if (reg_map.find(a) != reg_map.end() && reg_map[a] != "-1") // if %a has a physical register %exx assigned to it
            fprintf(stdout, "\tmovl %%eax, %%%s\n", reg_map[a].c_str());
        
        else if (reg_map.find(a) != reg_map.end()) // if %a is in memory
            fprintf(stdout, "\tmovl %%eax, %d(%%ebp)\n", offset_map[a]);
        
        else 
            printf(":: error in callStatements");
    }
    fprintf(stdout, "\tpopl %%edx\n");
    fprintf(stdout, "\tpopl %%ecx\n");
}

// done
// (br i1 %a, label %b, label %c) or (br label %b)
// a is instruction/condition, b and c are basic blocks
void brStatements(LLVMValueRef inst)
{
    if (LLVMGetNumOperands(inst) == 1) // 1 if branch is unconditional
    {
        LLVMBasicBlockRef a = (LLVMBasicBlockRef)LLVMGetOperand(inst, 0);
        string L = bb_labels[a];
        fprintf(stdout, "\tjmp %s\n", L.c_str());
    }
    else if (LLVMGetNumOperands(inst) == 3)
    {
        LLVMBasicBlockRef falseBB = (LLVMBasicBlockRef)LLVMGetOperand(inst, 1);
        LLVMBasicBlockRef trueBB = (LLVMBasicBlockRef)LLVMGetOperand(inst, 2);

        // basic blocks
        string L1 = bb_labels[trueBB];
        string L2 = bb_labels[falseBB];

        // predicate
        LLVMValueRef cond = LLVMGetOperand(inst, 0);
        LLVMIntPredicate T = LLVMGetICmpPredicate(cond);

        switch (T)
        {
            case LLVMIntSLT:
                fprintf(stdout, "\tjl %s\n", L1.c_str());
                break;
            case LLVMIntSGT:
                fprintf(stdout, "\tjg %s\n", L1.c_str());
                break;
            case LLVMIntSLE:
                fprintf(stdout, "\tjle %s\n", L1.c_str());
                break;
            case LLVMIntSGE:
                fprintf(stdout, "\tjge %s\n", L1.c_str());
                break;
            case LLVMIntEQ:
                fprintf(stdout, "\tje %s\n", L1.c_str());
                break;
            case LLVMIntNE:
                fprintf(stdout, "\tjne %s\n", L1.c_str());
                break;
            default:
                break;
        }
        fprintf(stdout, "\tjmp %s\n", L1.c_str());
    }
    else
    {
        printf("ERROR: Invalid branch instruction\n");
    }
}

// done
// (%a = add nsw A, B)
void mathStatements(LLVMValueRef a)
{
    // 6.1 Let R be the register for %a. If %a has a physical register %exx assigned to it, then R  is %exx, else R is %eax.
    string R = "eax";
    if (reg_map.find(a) != reg_map.end() && reg_map[a] != "-1") R = reg_map[a];

    LLVMValueRef A = LLVMGetOperand(a, 0);
    LLVMValueRef B = LLVMGetOperand(a, 1);

    unordered_map<int, string> op_math;
    op_math[LLVMICmp] = "cmpl";
    op_math[LLVMAdd] = "addl";
    op_math[LLVMMul] = "imull";
    op_math[LLVMSub] = "subl";
    
    /* A */
    if (LLVMIsConstant(A))
    {
        fprintf(stdout, "\tmovl $%lld, %%%s\n", LLVMConstIntGetSExtValue(A), R.c_str());
    }
    else if (funcParameter != A) // if A is a temporary variable
    {
        // If A has a physical register
        if (reg_map.find(A) != reg_map.end() && reg_map[A] != "-1")
        {
            if (reg_map[A] != R)
                fprintf(stdout, "\tmovl %%%s, %%%s\n", reg_map[A].c_str(), R.c_str());
            //else printf("reg_map[A] and R are same register"); //bp
        }
        // If A is in mem aka = -1
        else if (reg_map.find(A) != reg_map.end())
        {
            fprintf(stdout, "\tmovl %d(%%ebp), %%%s\n", offset_map[A], R.c_str());
        }
    }
    else {
        printf("error A is funcparam, not temp var or const");
    }

    /* B */
    if (LLVMIsConstant(B)) // if B is a constant
    {
        fprintf(stdout, "\t%s $%lld, %%%s\n", op_math[LLVMGetInstructionOpcode(a)].c_str(), LLVMConstIntGetSExtValue(B), R.c_str());
    }
    else if (funcParameter != B) // b is a temp var
    {
        // If B has a physical register
        if (reg_map.find(B) != reg_map.end() && reg_map[B] != "-1")
        {
            fprintf(stdout, "\t%s %%%s, %%%s\n", op_math[LLVMGetInstructionOpcode(a)].c_str(), reg_map[B].c_str(), R.c_str());
        }
        // If B is in mem, find offset
        else if (reg_map.find(B) != reg_map.end())
        {
            fprintf(stdout, "\t%s %d(%%ebp), %%%s\n", op_math[LLVMGetInstructionOpcode(a)].c_str(), offset_map[B], R.c_str());
        }
    }
    else {
        printf("error B is funcparam, not temp var or const");
    }

    /* %a in memory */
    if (reg_map.find(a) != reg_map.end() && reg_map[a] == "-1")
    {
        fprintf(stdout, "\tmovl %%eax, %d(%%ebp)\n", offset_map[a]);
    }
}

// done
// (%a = icmp slt A, B)
// A and B are operands; a is lhs so it's the instruction
void compareStatements(LLVMValueRef a)
{
    LLVMValueRef A = LLVMGetOperand(a, 0);
    LLVMValueRef B = LLVMGetOperand(a, 0);

    //7.1
    if (reg_map.find(a) == reg_map.end()) printf("error compareStatements\n");
    string R = reg_map[a];
    if (R == "-1") R = "eax";

    if (LLVMIsConstant(B)) // 7.2
        fprintf(stdout, "\tmovl $%lld, %%%s\n", LLVMConstIntGetSExtValue(B), R.c_str());
    else if (funcParameter != B && reg_map.find(B) != reg_map.end()) { // 7.3 if B is a temporary variable
        if (reg_map[B] != "-1" && reg_map[B] != R) //7.3.1
            fprintf(stdout, "\tmovl %%%s, %%%s\n", reg_map[B].c_str(), R.c_str()); //askv just checking all ebx etc should have %5
        else if (reg_map[B] == "-1") //7.3.2
            fprintf(stdout, "\tmovl %d(%ebp), %%%s\n", offset_map[B], R.c_str());
    }

    if (LLVMIsConstant(A)) 
        fprintf(stdout, "\tcmpl $%lld, %%%s\n", LLVMConstIntGetSExtValue(A), R.c_str()); //askv should R have % too? i added to add here 
    else if (funcParameter != A){
        if (reg_map.find(A) != reg_map.end() && reg_map[A] != "-1")  // 5
            fprintf(stdout, "\tcmpl %%%s, %%%s\n", reg_map[A].c_str(), R.c_str());
        else if (reg_map.find(A) != reg_map.end() && reg_map[A] == "-1") // 6 does not have a physical register assigned
            fprintf(stdout, "\tcmpl %d(%ebp), %%%s\n", offset_map[A], R.c_str());
    }

    if (reg_map.find(a) != reg_map.end() && reg_map[a] == "-1")
        fprintf(stdout, "\tmovl %eax, %d(%ebp)\n", offset_map[a]);
}
