# Variables
clang_flags = `llvm-config-15 --cxxflags --ldflags --libs core` -I /usr/include/llvm-c-15/ -ggdb -gdwarf-4 -g
test_file = ./final_tests/p1_opt.ll
test_c = ./final_tests/p1.c
output = test_file_output.txt
folder = final_tests/

all: codegenrun.cpp codegen.cpp
	clang++ ${clang_flags} codegenrun.cpp codegen.cpp -o codegenrun.out 
	clang ${folder}p1.c ${folder}main.c -o program

runa:
	./codegenrun.out ${test_file} > A.s
	gcc -g -m32 A.s -o A ${folder}main.c
	./A > A.txt

runb:
	./program > B.txt

clean:
	rm -f *.o codegenrun.out program A
