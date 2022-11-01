cppbuild: bin/main.o bin/CppBuilder.o bin/native_commands.o bin/compiler_commands.o bin/ProjectInfo.o
	g++ -o cppbuild bin/main.o bin/CppBuilder.o bin/native_commands.o bin/compiler_commands.o bin/ProjectInfo.o

bin/main.o: src/main.cpp bin
	g++ -c -std=c++17 -o bin/main.o src/main.cpp

bin/CppBuilder.o: src/CppBuilder.cpp bin
	g++ -c -std=c++17 -o bin/CppBuilder.o src/CppBuilder.cpp

bin/ProjectInfo.o: src/ProjectInfo.cpp bin
	g++ -c -std=c++17 -o bin/ProjectInfo.o src/ProjectInfo.cpp

bin/native_commands.o: src/native_commands.cpp bin
	g++ -c -std=c++17 -o bin/native_commands.o src/native_commands.cpp

bin/compiler_commands.o: src/compiler_commands.cpp bin
	g++ -c -std=c++17 -o bin/compiler_commands.o src/compiler_commands.cpp

bin: 
	mkdir bin