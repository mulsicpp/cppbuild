cppbuild: bin/main.o bin/CppBuilder.o bin/SystemInterface.o bin/ProjectInfo.o
	g++ -o cppbuild bin/main.o bin/CppBuilder.o bin/SystemInterface.o bin/ProjectInfo.o

bin/main.o: src/main.cpp bin
	g++ -c -std=c++17 -o bin/main.o src/main.cpp

bin/CppBuilder.o: src/CppBuilder.cpp bin
	g++ -c -std=c++17 -o bin/CppBuilder.o src/CppBuilder.cpp

bin/ProjectInfo.o: src/ProjectInfo.cpp bin
	g++ -c -std=c++17 -o bin/ProjectInfo.o src/ProjectInfo.cpp

bin/SystemInterface.o: src/SystemInterface.cpp bin
	g++ -c -std=c++17 -o bin/SystemInterface.o src/SystemInterface.cpp

bin: 
	mkdir bin