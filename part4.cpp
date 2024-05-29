#include "part4.h"

/* HELPER FUNCTIONS */
void compute_liveness(LLVMBasicBlockRef bb);
auto compare_instructions_func = [](LLVMValueRef inst1, LLVMValueRef inst2)
{
    return liveRange[inst1].second > liveRange[inst2].second;
};

// Emit functions
void returnStatements(LLVMValueRef inst);
void loadStatements(LLVMValueRef inst);
void storeStatements(LLVMValueRef inst, LLVMValueRef func_parameter);
void callStatements(LLVMValueRef inst, LLVMValueRef func);
void brStatements(LLVMValueRef inst);
void allocaStatements(LLVMValueRef inst);
void aritmeticStatements(LLVMValueRef inst);
void compareStatements(LLVMValueRef inst);

/* ASSEMBLY CODE */
void createBBLabels(LLVMModuleRef module);
void printDirectives(LLVMValueRef func) void printFunctionEnd();
void getOffsetMap(LLVMValueRef func);

/* VARIABLES */
unordered_map<LLVMValueRef, string> reg_map;
unordered_map<LLVMValueRef, int> offset_map;

// instructions
unordered_map<LLVMValueRef, int> inst_index;
vector<LLVMValueRef> sorted_insts;
unordered_map<LLVMValueRef, pair<int, int>> live_range;

// operand, opcode
unordered_set<int> no_result_operands = {LLVMBr, LLVMRet, LLVMCall, LLVMStore};
unordered_set<LLVMOpcode> math_opcodes = {LLVMAdd, LLVMSub, LLVMMul};

// basic block
unordered_map<LLVMBasicBlockRef, pair<int, LLVMValueRef>> bb_labels; // bb - bb index and first bb instruction //todo change to string

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
    createBBLabels();
    for (LLVMValueRef func = LLVMGetFirstFunction(module); func != NULL; func = LLVMGetNextFunction(func))
    {                          // 1
        printDirectives(func); // 2
        getOffsetMap(func);    // 3

        LLVMValueRef funcParameter; // todo do i need this
        if (LLVMCountParams(func) > 0)
        {
            funcParameter = LLVMGetFirstParam(func);
        }

        // 4-7
        fprintf(stdout, "\t pushl %%ebp\n");
        fprintf(stdout, "\t movl %%esp, %%ebp\n");
        fprintf(stdout, "\t subl todo penguin,%%esp\n"); // todo local memory
        fprintf(stdout, "\t  pushl %%ebx\n");

        // 8
        for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(func); bb != NULL; bb = LLVMGetNextBasicBlock(bb))
        {
            pair<int, LLVMValueRef> bbInfo = bb_labels[bb];

            // 8.1
            fprintf(stdout, ".L%d:\n", bbInfo.first); // Define label for the basic block

            // 8.2
            for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst != NULL; inst = LLVMGetNextInstruction(inst))
            {
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
                    aritmeticStatements(inst);
                    break;
                case LLVMSub:
                    aritmeticStatements(inst);
                    break;
                case LLVMMul:
                    aritmeticStatements(inst);
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
            bb_labels[bb] = make_pair(bbCount, LLVMGetFirstInstruction(bb));
            bbCount++;
        }
    }
}

void printDirectives(LLVMValueRef func)
{
    fprintf(stdout, "\t.text\n");
    fprintf(stdout, "\t.globl %s\n", LLVMGetValueName(func));
    fprintf(stdout, "\t.type %s, @function\n", LLVMGetValueName(func));
    fprintf(stdout, "%s:\n", LLVMGetValueName(func));
    fprintf(stdout, "\tpushl %%ebp\n");
    fprintf(stdout, "\tmovl %%esp, %%ebp\n");
    fprintf(stdout, "\tsubl $%d, %%esp\n", stackOffset);
}

void printFunctionEnd()
{
    fprintf(stdout, "\t movl %%ebp, %%esp\n");
    fprintf(stdout, "\t popl %%ebp\n");
    fprintf(stdout, "\tleave\n");
    fprintf(stdout, "\tret\n");
}

void getOffsetMap(LLVMValueRef func)
{
    offset_map.clear();

    int offset = 0;
    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(func); bb != NULL; bb = LLVMGetNextBasicBlock(bb))
    {
        for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst != NULL; inst = LLVMGetNextInstruction(inst))
        {
            if (LLVMGetInstructionOpcode(inst) == LLVMAlloca)
            {
                offset_map[inst] = offset;
                offset += 4;
            }
        }
    }
    for (pair<LLVMValueRef, string> reg : reg_map)
    {
        if (reg.second != "-1")
        {
            offset_map[reg.first] = offset;
            offset += 4;
        }
    }
    stackOffset = offset;
}

//reg is -1 or spill then var is in memory
bool isTemporaryVariable(LLVMValueRef v) {
    return reg_map.find(v) != reg_map.end() && reg_map[v] != "-1";
}
// like (ret i32 A)
void returnStatements(LLVMValueRef inst)
{
    LLVMValueRef A = LLVMGetOperand(inst, 0);

    if (LLVMIsConstant(A)) // 1
    {
        fprintf(stdout, "\tmovl $%lld, %%eax\n", LLVMConstIntGetSExtValue(A));
    }
    // if A is a temporary variable and is in memory
    else if (isTemporaryVariable(A)) // 2
    {
        int k = offset_map[A];
        fprintf(stdout, "\tmovl %d(%%ebp), %%eax\n", k);
    }
    // if A is a temporary variable and has a physical register
    else if (reg_map.find(A) != reg_map.end()) // 3
    {
        fprintf(stdout, "\tmovl %%%s, %%eax\n", reg_map[A].c_str());
    }
    fprintf(stdout, "\tpopl %%ebx\n"); // 4
    printFuncEnd();                    // 5
}

// like (%a = load i32, i32* %b)
void loadStatements(LLVMValueRef a)
{
    // %b is the alloc instruction
    // %a is the load


    LLVMValueRef b = LLVMGetOperand(a, 0); // askv bp here; how to get %b //operand is instructiong we r working on 

    if (isTemporaryVariable(a)) 
    {
        string reg = reg_map[a];
        int c = offset_map[b];
        fprintf(stdout, "\tmovl %d(%%ebp), %%%s\n", c, reg.c_str());
    }
}

// %6 = load i32, ptr %3, align 4 todo
// 1 operand; %6 is the instruction itself

// store i32 %0, ptr %2, align 4
// 2 operands; since store doesn't have a creation no LHS

// like (store i32  A, i32* %b)
void storeStatements(LLVMValueRef inst, LLVMValueRef funcParam)
{
    LLVMValueRef A = LLVMGetOperand(inst, 0);
    if (funcParam == A)
        return;                 // 1
    else if (LLVMIsConstant(A)) // 2
    {
        LLVMValueRef b = LLVMGetOperand(inst, 1); // askv how to get %b? not getoperand?
        int c = offset_map[b];
        fprintf(stdout, "\tmovl $%lld, %d(%%ebp)\n", LLVMConstIntGetSExtValue(A), c);
        return;
    }
    else // 3 //askv how to check If A is a temporary variable %a
    {
        // bp w func parameter %0
        if (reg_map[A] != "-1")
        {
            string reg = reg_map[A];
            LLVMValueRef B = LLVMGetOperand(inst, 1);
            int c = offset_map[B]; // askv what is B vs b? how to get %b?
            fprintf(stdout, "\tmovl %%%s, %d(%%ebp)\n", reg.c_str(), c);
        }
        else
        {
            LLVMValueRef b = LLVMGetOperand(inst, 1);
            int c1 = offset_map[A];
            fprintf(stdout, "\tmovl %d(%%ebp), %%eax\n", c1);

            int c2 = offset_map[b];
            fprintf(stdout, "\tmovl %%eax, %d(%%ebp)\n", c2);
        }
    }
}

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
        else
        {
            // askv is this how to check if P is a temporary variable and has a physical register
            // temp var is related to llvm
            // llvm has no physical registers
            // when translating into machine code, we assign a temp variable a physical register
            /*
                check if the parameter P is a temporary var %p
                aka check if it's an instruction
                
                checking physical register is if i assigned something

                todo %p is in memory reg_map if its value is -1; 
                > IF YOU HAVE IN REGISTER it's not being used from the memory

                #todo add all instructions to map as -1
            */

            if (reg_map.find(P) != reg_map.end() && reg_map[P] != "-1")
                fprintf(stdout, "\tpushl %%%s\n", reg_map[P].c_str());
            else if (reg_map.find(P) != reg_map.end()) // todo If p is in memory askv what is diff between 5p and P
            {
                int k = offset_map[P];
                fprintf(stdout, "\tpushl %d(%%ebp)\n", k);
            }
        }
    }

    fprintf(stdout, "\tcall func\n"); // 4

    if (parameterCount == 1) // 5
        fprintf(stdout, "\taddl $4, %%esp\n");

    // todo if Instr is of the form (%a = call type @func())
    if (LLVMGetNumOperands(inst) == 1)
    {
        LLVMValueRef a = LLVMGetOperand(inst, 0);
        if (reg_map.find(a) != reg_map.end() && reg_map[a] != "-1") // if %a has a physical register %exx assigned to it
        {
            fprintf(stdout, "\tmovl %%eax, %%%s\n", reg_map[a].c_str());
        }
        else if (reg_map.find(a) != reg_map.end())
        {
            int k = offset_map[a];
            fprintf(stdout, "\tmovl %%eax, %d(%%ebp)\n", k);
        }
    }
    fprintf(stdout, "\tpopl %%edx\n");
    fprintf(stdout, "\tpopl %%ecx\n");
}

//(br i1 %a, label %b, label %c) or (br label %b)
void brStatements(LLVMValueRef inst)
{
    if (LLVMGetNumOperands(inst) == 1) // 1 if branch is unconditional
    {
        LLVMBasicBlockRef a = (LLVMBasicBlockRef)LLVMGetOperand(inst, 0);
        pair<int, LLVMValueRef> bbInfo = bb_labels[a];
        fprintf(stdout, "\tjmp %d\n", bbInfo.first);
    }
    else if (LLVMGetNumOperands(inst) == 3)
    {
        LLVMBasicBlockRef b = (LLVMBasicBlockRef)LLVMGetOperand(inst, 1);
        LLVMBasicBlockRef c = (LLVMBasicBlockRef)LLVMGetOperand(inst, 2);
        int L1 = bb_labels[b].first;
        int L2 = bb_labels[c].first;

        LLVMValueRef cond = LLVMGetOperand(inst, 0);
        LLVMIntPredicate T = LLVMGetICmpPredicate(cond);
        switch (T)
        {
            case LLVMIntSLT:
                fprintf(stdout, "\tjl %d\n", L1); //askv by emit L1, does that just mean the integer? then change for all below //todo label with string bb1, bb2 etc so L1 should be string
                break;
            case LLVMIntSGT:
                fprintf(stdout, "\tjg %d\n", L1);
                break;
            case LLVMIntSLE:
                fprintf(stdout, "\tjle %d\n", L1);
                break;
            case LLVMIntSGE:
                fprintf(stdout, "\tjge %d\n", L1);
                break;
            case LLVMIntEQ:
                fprintf(stdout, "\tje %d\n", L1);
                break;
            case LLVMIntNE:
                fprintf(stdout, "\tjne %d\n", L1);
                break;
            default:
                break;
        }
        fprintf(stdout, "\tjmp L%d\n", L2);
    }
    else
    {
        printf("ERROR: Invalid branch instruction\n");
    }
}

// (%a = add nsw A, B)
void aritmeticStatements(LLVMValueRef a)
{
    // 6.1 Let R be the register for %a. If %a has a physical register %exx assigned to it, then R  is %exx, else R is %eax.
    string R = "eax";

    LLVMValueRef A = LLVMGetOperand(a, 0);
    LLVMValueRef B = LLVMGetOperand(a, 1);

    unordered_map<int, string> op_math;
    op_math[LLVMICmp] = "cmpl";
    op_math[LLVMAdd] = "addl";
    op_math[LLVMMul] = "imull";
    op_math[LLVMSub] = "subl";

    // if a has a physical register, use it or default to eax
    if (reg_map.find(a) != reg_map.end() && reg_map[a] != "-1")
    {
        R = reg_map[a];
    }

    // Handle OP1
    if (LLVMIsConstant(A))
    {
        fprintf(stdout, "\tmovl $%lld, %%%s\n", LLVMConstIntGetSExtValue(A), R.c_str());
    }
    else
    {
        // If A has a physical register
        if (reg_map.find(A) != reg_map.end() && reg_map[A] != "-1")
        {
            fprintf(stdout, "\tmovl %%%s, %%%s\n", reg_map[A].c_str(), R.c_str());
        }
        // If A is in mem, find offset
        else if (reg_map.find(A) != reg_map.end())
        {
            int offset = offset_map[A];
            fprintf(stdout, "\tmovl %d(%%ebp), %%%s\n", offset, R.c_str());
        }
    }

    // Handle OP2
    if (LLVMIsConstant(B))
    {
        fprintf(stdout, "\t%s $%lld, %%%s\n", op_math[LLVMGetInstructionOpcode(a)].c_str(), LLVMConstIntGetSExtValue(B), R.c_str());
    }
    else
    {
        // If B has a physical register
        if (reg_map.find(B) != reg_map.end() && reg_map[B] != "-1")
        {
            fprintf(stdout, "\t%s %%%s, %%%s\n", op_math[LLVMGetInstructionOpcode(a)].c_str(), reg_map[B].c_str(), R.c_str());
        }
        // If B is in mem, find offset
        else if (reg_map.find(B) != reg_map.end())
        {
            int offset = offset_map[B];
            fprintf(stdout, "\t%s %d(%%ebp), %%%s\n", op_math[LLVMGetInstructionOpcode(a)].c_str(), offset, R.c_str());
        }
    }

    // Handle destination
    if (reg_map.find(a) != reg_map.end())
    {
        int offset = offset_map[a];
        fprintf(stdout, "\tmovl %%eax, %d(%%ebp)\n", offset);
    }
}

// (%a = icmp slt A, B)
// A and B are operands; a is lhs so it's the instruction
void compareStatements(LLVMValueRef a)
{
    LLVMValueRef A = LLVMGetOperand(a, 0); //askv on operand v pointer; correct
    LLVMValueRef B = LLVMGetOperand(a, 0);
    string R = reg_map[A];
    if (R == "") R = "%eax"; // 7.1
    if (LLVMIsConstant(B)) fprintf(stdout, "\tmovl %s,%s\n", LLVMConstIntGetSExtValue(B), R.c_str()); //askv what does $B mean? actually PRINT $B
    if (isTemporaryVariable(B)) {
        if (reg_map[B])
    }
    //penguin

    // Let R be the register for %a. If %a has a physical register %exx assigned to it, then R  is %exx, else R is %eax.
    //  if B is a constant
    // emit movl $B, R
    //  if B is a temporary variable
    // if B has physical register %eyy assigned to it:
    // emit movl %eyy, R  (do not emit if both registers are the same)
    // if B is in memory:
    // get offset n of B
    // emit movl n(%ebp), R
    //  if A is a constant
    // emit cmpl $A, R
    // if A is a temporary variable and has physical register %ezz assigned to it:
    // emit cmpl %ezz, R
    // if A is a temporary variable and  does not have a physical register assigned to it:
    // get offset m of A
    // emit cmpl m(%ebp), R
    // if %a is in memory
    // get offset k of %a
    // emit movl %eax, k(%ebp)
}
