#include "../Global/Communicator.h"
#include <iostream>
#include <conio.h>
#include "../ROUTER/Router.h"

#define GET_COMMAND "GET"
#define PUT_COMMAND "PUT"

void bp(char *msg, bool block=true){
	std::cout << "BREAK - " << msg << std::endl;
	if(block) getch();
}

void doGet(Communicator &comm, const char *path){
	comm.sendFile(path);
}

void doPut(Communicator &comm, const char *path){
	comm.recvFile(path);
}

void command(Communicator &comm, string &command){
	if (command.compare(GET_COMMAND) == 0){
		string filename;
		Packet p = comm.recvPacket(60 * 1000);
		filename = p.getMessageString();
		doGet(comm, filename.c_str());
	}

	if (command.compare(PUT_COMMAND) == 0){
		string filename;
		Packet p = comm.recvPacket(60 * 1000);
		filename = p.getMessageString();
		doPut(comm, filename.c_str());
	}
}

int main(){
	try{
		// WSA Initialization
		Communicator comm(PEER_PORT2);

		while (1){
			// Listening for hand shake
			comm.init();
			bp("Hand shake complete", false);


			try{
				Packet cmd = comm.recvPacket(60 * 1000); // Expecting a command within 1 minute
				command(comm, cmd.getMessageString());
				break;
				//doGet(comm, "./image.jpg");
			}
			catch (int){
				std::cerr << "Timeout: no command received" << std::endl;
			}
		}
	}
	catch (std::exception e){
		std::cerr << e.what() << std::endl;
	}
	bp("Terminating");
}