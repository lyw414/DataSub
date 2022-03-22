all:
	g++ -fPIC -c ./src/DataSub.cpp -I ./src
	g++ -shared -o libDataSub.so DataSub.o
	g++ -o test test.cpp -L./ -lDataSub -I ./src
