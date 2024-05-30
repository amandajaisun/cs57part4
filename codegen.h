#include <stdio.h>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <list>
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>
#include <cstddef>
#include <queue>
#include <vector>
#include <deque>
#include <string.h>
#include <set>
#include <stdio.h>
#include <string>
#include <algorithm>

#include "ast.h"

using namespace std;

void allocateRegisters(LLVMModuleRef m);
void codegen(LLVMModuleRef module);