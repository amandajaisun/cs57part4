#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>
#include "codegen.h"
#include <stdio.h>

int main(int totalArgs, char **args)
{
    if (totalArgs < 2)
    {
        printf("Usage: %s <filename>\n", args[0]);
        return -1;
    }

    // Creation of module and builder
    LLVMModuleRef myModule = LLVMModuleCreateWithName("myModule");
    LLVMBuilderRef myBuilder = LLVMCreateBuilder();

    // Loading IR file 
    LLVMMemoryBufferRef memoryBuffer;
    char *errorMsg = NULL;

    if (LLVMCreateMemoryBufferWithContentsOfFile(args[1], &memoryBuffer, &errorMsg))
    {
        printf("error LLVMCreateMemoryBufferWithContentsOfFile");
        LLVMDisposeMessage(errorMsg);
        return -1;
    }

    // Parsing the in-memory IR
    if (LLVMParseIRInContext(LLVMGetGlobalContext(), memoryBuffer, &myModule, &errorMsg))
    {
        printf("error LLVMParseIRInContext");
        LLVMDisposeMessage(errorMsg);
        return -1;
    }

    allocateRegisters(myModule);
    codegen(myModule);

    return 0; 
}
