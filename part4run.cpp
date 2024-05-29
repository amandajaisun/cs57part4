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
        return 1;
    }

    // Creation of module and builder
    LLVMModuleRef myModule = LLVMModuleCreateWithName("test");
    LLVMBuilderRef myBuilder = LLVMCreateBuilder();

    // Loading IR file 
    LLVMMemoryBufferRef memoryBuffer;
    char *errorMsg = NULL;

    if (LLVMCreateMemoryBufferWithContentsOfFile(args[1], &memoryBuffer, &errorMsg))
    {
        fprintf(stderr, "error loading file %s: %s\n", args[1], errorMsg);
        LLVMDisposeMessage(errorMsg);
        return 1;
    }

    // Parsing the in-memory IR
    if (LLVMParseIRInContext(LLVMGetGlobalContext(), memoryBuffer, &myModule, &errorMsg))
    {
        fprintf(stderr, "error parsing IR: %s\n", errorMsg);
        LLVMDisposeMessage(errorMsg);
        return 1;
    }

    registerAllocation(myModule);
    codegen(myModule);

    return 0; 
}
