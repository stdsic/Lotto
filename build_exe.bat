g++ -static-libgcc -static-libstdc++ -c -o main.o main.cpp -mwindows -municode
g++ -static-libgcc -static-libstdc++ -o Lotto.exe -s main.o resource.o -mwindows -municode
