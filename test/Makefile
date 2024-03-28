all:client2
client1:client1.cpp
	g++ -g -std=c++11 $^ -o $@
client2:client2.cpp
	g++ -g -std=c++11 $^ -o $@
client:tcp_cli.cc
	g++ -g -std=c++11 $^ -o $@
server:server.cc
	g++ -g -std=c++11 -pthread $^ -o $@	
tcp_srv:tcp_srv.cc
	g++ -g -std=c++11 -pthread $^ -o $@	
.PHONY:clean
clean:
	rm -rf client1
