# Variables
# source = part4run
# clang_flags = `llvm-config-15 --cxxflags --ldflags --libs core` -I /usr/include/llvm-c-15/ -ggdb -gdwarf-4 -g
# test_file = ./final_tests/p1_opt.ll
# test_c = ./final_tests/p1.c
# output = test_file_output.txt
# folder = final_tests/

# run: ${source}.c++ codegen.c++
# 	clang++ ${clang_flags} -o ${source}.out ${source}.c++ codegen.c++
# 	clang ${folder}p1.c ${folder}main.c -o program
# 	./${source}.out ${test_file} > A.txt
# 	./program > B.txt
# Variables
source = part4run
LLVM_INCLUDES = -I/usr/include/llvm-c-15/
LLVM_LIBS = -lLLVM-15
test_file = p2_common_subexpr.ll #./final_tests/p1_opt.ll
test_c = p2_common_subexpr.c
main_c = main.c
output = test_file_output.txt
folder = final_tests/

# Targets
run: ${source}.c++ codegen.c++ codegen.h ${test_file}
	# Compile part4run and codegen with 32-bit mode
	clang++ ${LLVM_INCLUDES} -o ${source}.out ${source}.c++ codegen.c++

	# Run part4run with the provided .ll file and generate assembly
	./${source}.out ${test_file} > A.s

	# Compile the generated assembly and main.c to produce the executable in 32-bit mode
	clang -m32 ${LLVM_INCLUDES} -o my-part4-executable A.s ${main_c}

	# Run the generated executable
	./my-part4-executable > B.txt

	# Compile p1.c and main.c to produce the executable from the source in 32-bit mode
	clang -m32 ${LLVM_INCLUDES} ${test_c} ${main_c} -o clang-source-executable

	# Run the source-based executable
	./clang-source-executable > clang-source-output

	# Compare the outputs
	diff B.txt clang-source-output

clean:
	rm -f ${source}.out A.s my-part4-executable clang-source-executable B.txt clang-source-output
