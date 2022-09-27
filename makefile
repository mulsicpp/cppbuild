cppbuild: bin/main.o bin/CppBuilder.o bin/native_commands.o
	g++ -o cppbuild bin/main.o bin/CppBuilder.o bin/native_commands.o

bin/main.o: src/main.cpp bin
	g++ -c -std=c++17 -o bin/main.o src/main.cpp

bin/CppBuilder.o: src/CppBuilder.cpp bin
	g++ -c -std=c++17 -o bin/CppBuilder.o src/CppBuilder.cpp

bin/native_commands.o: src/native_commands.cpp bin
	g++ -c -std=c++17 -o bin/native_commands.o src/native_commands.cpp

bin: 
	mkdir bin