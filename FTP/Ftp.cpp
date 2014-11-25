#include "../Global/Communicator.h"
#include <iostream>
#include <sstream>
#include "../ROUTER/Router.h"
#include <conio.h>

#define GET_COMMAND "GET"
#define PUT_COMMAND "PUT"

void bp(char *msg, bool block=true){
	std::cout << "BREAK - " << msg << std::endl;
	if(block) getch();
}

void doGet(Communicator &comm, const char *path){
	comm.recvFile(path);
}

void doPut(Communicator &comm, const char *path){
	comm.sendFile(path);
}

void prompt(Communicator &comm){
	string cmd;
	std::cout << "Enter command [get|put]: ";
	std::cin >> cmd;
	if (cmd.compare("get") == 0){
		comm.sendPacket(Packet(GET_COMMAND));
		string filename;
		std::cout << "Enter filename: ";
		std::cin >> filename;
		comm.sendPacket(Packet(filename.c_str()));
		doGet(comm, filename.c_str());
	}

	if (cmd.compare("put") == 0){
		comm.sendPacket(Packet(PUT_COMMAND));
		string filename;
		std::cout << "Enter filename: ";
		std::cin >> filename;
		comm.sendPacket(Packet(filename.c_str()));
		doPut(comm, filename.c_str());
	}
}

int main(){
	try{
		// WSA Initialization
		Communicator comm(PEER_PORT1);

		// Get server address
		// FIXME: default hostname HARDCODE for debug
		string server_name("Cipolla");
		//std::cout << "Enter Server hostname: ";
		//std::cin >> server_name;
		struct hostent *server = NULL;
		server = gethostbyname(server_name.c_str());
		if (!server){
			std::cerr << server_name << " not found " << std::endl;
			exit(EXIT_FAILURE);
		}
		sockaddr_in server_address;
		std::memset((char *)&server_address, 0, sizeof(server_address));
		server_address.sin_family = AF_INET;
		std::memcpy((void *)&server_address.sin_addr, server->h_addr_list[0], server->h_length);
		server_address.sin_port = htons(ROUTER_PORT1);

		// Initiate hand shake
		comm.init(&server_address);
		bp("Hand shake complete", false);

		prompt(comm);
	}
	catch(std::exception e){
		std::cerr << e.what() << std::endl;
	}
	bp("Terminating");
}