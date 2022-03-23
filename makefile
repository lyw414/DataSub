all:
	g++ -fPIC -c ./src/IPC.cpp -I ./src
	g++ -fPIC -c ./src/SubscribeSvr.cpp -I ./src
	g++ -fPIC -c ./src/SubscribeTaskBase.cpp -I ./src
	g++ -fPIC -c ./src/DataSub.cpp -I ./src
	g++ -fPIC -c ./src/PublishSvr.cpp -I ./src
	g++ -fPIC -c ./src/PublishTaskBase.cpp -I ./src
	g++ -shared -o libDataSub.so DataSub.o SubscribeSvr.o  SubscribeTaskBase.o PublishSvr.o PublishTaskBase.o IPC.o -I ./src
	rm DataSub.o SubscribeSvr.o  SubscribeTaskBase.o PublishSvr.o PublishTaskBase.o IPC.o 
	g++ -o test test.cpp -L./ -lDataSub -I ./src
