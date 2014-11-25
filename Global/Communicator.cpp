#include "Communicator.h"
#include <sstream>
#include <iostream>
#include <WS2tcpip.h>
#include <ctime>
#include "PacketProvider.h"

#define __VERBOSE__

Communicator::Communicator(int port){
	WSADATA   api_info;
	addrinfo  hints;
	char      localhost[128];
	int       res;

	// Start Window socket API
	if ((res = WSAStartup(MAKEWORD(2, 2), &api_info)) != 0){
		std::stringstream ss;
		ss << "Unable to initialize WSA: " << res;
		throw std::exception(ss.str().c_str());
	}

	// Display name of local host.
	gethostname(localhost, sizeof(localhost));
	std::cout << "Communicator starting at host: " << localhost << std::endl;

	// Create a new socket
	this->socket = INVALID_SOCKET;
	this->socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (this->socket == INVALID_SOCKET){
		WSACleanup();
		std::stringstream ss;
		ss << "Unable to create a socket";
		throw std::exception(ss.str().c_str());
	}

	// Get local hostname
	struct hostent *local = NULL;
	local = gethostbyname(localhost);
	if (!local){
		std::stringstream ss;
		ss << localhost << " not found ";
		closesocket(socket);
		WSACleanup();
		throw std::exception(ss.str().c_str());
	}

	//Bind socket
	std::memset((char *)&local_address, 0, sizeof(local_address));
	local_address.sin_family = AF_INET;
	std::memcpy((void *)&local_address.sin_addr, local->h_addr_list[0], local->h_length);
	local_address.sin_port = htons(port);
	if (bind(socket, (struct sockaddr *)&local_address, sizeof(local_address)) < 0) {
		std::stringstream ss;
		ss << "bind failed with error: " << WSAGetLastError();
		closesocket(socket);
		WSACleanup();
		throw std::exception(ss.str().c_str());
	}
	std::cout << "Binding Communicator to port " << port << std::endl;
	
	srand(time(0));
	this->index = rand() % MAX_INDEX;
	this->remote_index = 0;
	std::memset((char *)&remote_address, 0, sizeof(remote_address));
}

Communicator::~Communicator(){
	closesocket(socket);
	WSACleanup();
}

bool Communicator::init(sockaddr_in *destination){
	if (destination == NULL){
		this->respondHandShake();
	}
	else{
		this->initiateHandShake(destination);
		std::memcpy((char *)&remote_address, destination, sizeof(remote_address));
	}
	return true;
}

bool Communicator::sendPacket(Packet &p, int attempts){
	p.setId(this->index);
	TIMEVAL timeout = getTimeout();
	int tries = 0;
	while (tries < attempts){
		// Send message
		sendto(&remote_address, p);
		try{
			// Wait for ACK
			Packet reply = recvfrom(&timeout);
			if (reply.isAck() && p.getId() == reply.getId()){
				increaseIndex();
				return true; // Success
			}
			else
				tries++; // Failure
		}
		catch (int){
			tries++;
		}
	}
	return false;
}

Packet Communicator::recvPacket(int timeout_ms){
	Packet p;
	TIMEVAL timeout = getTimeout(timeout_ms);

	// Receive message
	int attempts = 0;
	while (attempts < MAX_ATTEMPTS){
		try{
			p = recvfrom(&timeout);
			if (p.getId() == ((this->remote_index + 1) % MAX_INDEX))
				break;
		}
		catch (int){
			attempts++;
		}
	}
	if (attempts == MAX_ATTEMPTS){ // Failure
		std::stringstream ss;
		ss << "Exceeding number of attempts (" << attempts << ")";
		throw std::exception(ss.str().c_str());
	}

	// Send ACK
	timeout = getTimeout();
	attempts = 0;
	Packet ack = Packet::makeAck(p.getId());
	while (attempts < MAX_ATTEMPTS){
		try{
			sendto(&remote_address, ack);
			Packet reply = recvfrom(&timeout);
			if (reply.getId() == p.getId()) // Failure. it didn't receive the ACK
				attempts++;
		}
		catch (int){
			increaseRemoteIndex();
			return p; // Success
		}
	}
	throw std::exception("No ACK received"); // Failure
}

void Communicator::sendFile(string pathname, int attempts) {
	FilePacketProvider *factory_packet;
	factory_packet = new FilePacketProvider(pathname.c_str(), MAX_PACKET_SIZE);
	WindowManager mng(WINDOW_SIZE);
	// Fill manager
	std::vector<Packet> vec = factory_packet->get(mng.getAvailableSpots());
	for (std::vector<Packet>::iterator it = vec.begin(); it != vec.end(); it++){
		it->setId(this->index);
		increaseIndex();
	}
	mng.insert(vec);

	int tries = 0;
	try{
		while (!(factory_packet->isCompleted() && mng.isEmpty()) && (tries < attempts)){
			if(mng.getAvailableSpots() > 0){
				vec = factory_packet->get(mng.getAvailableSpots());
				for (std::vector<Packet>::iterator it = vec.begin(); it != vec.end(); it++){
					it->setId(this->index);
					increaseIndex();
				}
				mng.insert(vec);
			}
			std::vector<Packet> packets = mng.getPackets();
			for (std::vector<Packet>::iterator it = packets.begin(); it != packets.end(); it++){
				sendto(&remote_address, *it);
			}

			int recv_ack = 0;
			try{
				TIMEVAL timeout = getTimeout(TIMEOUT / 10); // try 10 times before the client gives up
				for (recv_ack = 0; recv_ack < packets.size(); recv_ack++){
					Packet p = recvfrom(&timeout);
					mng.ack(p);
					tries = 0;
				}
			}
			catch (int) { 
				if (!recv_ack) tries++; 
				continue;
			}
		}
	}
	catch (std::exception e){
		std::cerr << e.what() << std::endl;
	}
	if(tries >= attempts){
		std::stringstream ss;
		ss << "No response after " << tries << " tries";
		throw std::exception(ss.str().c_str());
	}

	delete factory_packet;
}

void Communicator::recvFile(string pathname, int timeout_ms){
	std::ofstream os(pathname, std::ios::binary);
	if (os.fail())
		throw std::exception("Cannot write file");
	
	TIMEVAL timeout = getTimeout(timeout_ms);
	while (true){
		try{
			Packet p = recvfrom(&timeout);
			
			if (p.getId() == ((this->remote_index + 1) % MAX_INDEX)){
				increaseRemoteIndex();
				sendto(&remote_address, Packet::makeAck(p.getId()));
				os.write(p.getMessage(), p.getMessageLength());
			}
		}
		catch (int){ break; }
	}

	os.close();
}

int Communicator::getLocalIndex(){
	return this->index;
}

int Communicator::getRemoteIndex(){
	return this->remote_index;
}

void Communicator::sendto(sockaddr_in *destination, Packet& message){
	char *buffer = new char[MAX_BUFFER_SIZE];
	size_t buffer_length = message.encode(&buffer, MAX_BUFFER_SIZE);
	int err = ::sendto(socket, buffer, buffer_length, 0, (sockaddr *)destination, sizeof(*destination));
#ifdef __VERBOSE__
	std::cout << "sending Packet [" << message.getId() << " | data(" << message.getMessageLength() << ") ]" << std::endl;
#endif
	if (err == SOCKET_ERROR){
		std::stringstream ss;
		ss << "WSA Error: " << WSAGetLastError();
		throw std::exception(ss.str().c_str());
	}
}

Packet Communicator::recvfrom(TIMEVAL *timeout){
	fd_set fd_read;
	FD_ZERO(&fd_read);
	FD_SET(this->socket, &fd_read);
	int err = select(0, &fd_read, NULL, NULL, timeout);
	if (err == SOCKET_ERROR){
		std::stringstream ss;
		ss << "Socket Error: " << WSAGetLastError();
		throw std::exception(ss.str().c_str());
	}
	if (err == 0){
		std::stringstream ss;
		ss << "Timeout";
		throw 1;
	}
	char *buffer = new char[MAX_BUFFER_SIZE];
	int remote_address_size = sizeof(sockaddr);
	err = ::recvfrom(socket, buffer, MAX_BUFFER_SIZE, 0, (sockaddr *) &remote_address, &remote_address_size);
	if (err == SOCKET_ERROR){
		std::stringstream ss;
		ss << "WSA Error: " << WSAGetLastError();
		throw std::exception(ss.str().c_str());
	}
	Packet p;
	p.decode(buffer, err);
#ifdef __VERBOSE__
	std::cout << "recving Packet [" << p.getId() << " | data(" << p.getMessageLength() << ") ]" << std::endl;
#endif
	return p;
}

void Communicator::initiateHandShake(sockaddr_in *destination){
	Packet p1(this->index, "SYN", 3); increaseIndex();// P1: sending packet with index as id

	TIMEVAL timeout = getTimeout();
	int attempt = 0;
	while (attempt < MAX_ATTEMPTS){
		this->sendto(destination, p1);
		try{
			Packet p2 = this->recvfrom(&timeout); // P2: receiving packet with id = P1 and message as the receiver index
			if (p2.getId() == p1.getId()){
				std::stringstream ss;
				ss << p2.getMessageString();
				ss >> this->remote_index;
				this->sendto(destination, Packet::makeAck(p2.getId())); // P3: Simple ACK
				break;
			}
			else
				throw std::exception("ACK Packet do not match SYN Packet");
		}
		catch (int){
			attempt++;
			continue;
		}
	}
	if (attempt >= MAX_ATTEMPTS){
		std::stringstream ss;
		ss << "No response after " << MAX_ATTEMPTS << " tries";
		throw std::exception(ss.str().c_str());
	}
}

void Communicator::respondHandShake(){
	Packet p1 = recvfrom(); // Blocking call P1
	if (p1.getMessageString().compare("SYN") != 0 && p1.getMessageString().length() != 3){
		std::stringstream ss;
		ss << "SYN packet not received aborting";
		throw std::exception(ss.str().c_str());
	}
	this->remote_index = p1.getId();
	std::stringstream ss;
	ss << this->index;
	Packet p2(p1.getId(), ss.str().c_str(), ss.str().length()); increaseIndex(); // P2
	
	TIMEVAL timeout = getTimeout();
	int attempt = 0;
	while (attempt < MAX_ATTEMPTS){
		sendto(&remote_address, p2);
		try{
			Packet p3 = recvfrom(&timeout); // P3
			if (p3.isAck()) break;
		}
		catch (int){
			attempt++;
		}
	}
	if (attempt >= MAX_ATTEMPTS){
		std::stringstream ss;
		ss << "No response after " << MAX_ATTEMPTS << " tries";
		throw std::exception(ss.str().c_str());
	}
}

void Communicator::increaseIndex(){
	this->index = ++index % MAX_INDEX;
}

void Communicator::increaseRemoteIndex(){
	this->remote_index = ++remote_index % MAX_INDEX;
}

TIMEVAL Communicator::getTimeout(int millisecond){
	TIMEVAL timeout;
	if (millisecond != 0) {
		timeout.tv_sec = 0;
		timeout.tv_usec = millisecond * 1000;
	}
	else{
		timeout.tv_sec = 0;
		timeout.tv_usec = TIMEOUT * 1000;
	}
	return timeout;
}

Communicator::WindowManager::WindowManager(int window_size):
size(window_size),
spots(0)
{
	this->window = new Packet[size];
}

Communicator::WindowManager::~WindowManager(){
	delete[] window;
}

bool Communicator::WindowManager::isEmpty(){
	return this->spots == 0;
}

int Communicator::WindowManager::getAvailableSpots(){
	return this->size - this->spots;
}

void Communicator::WindowManager::insert(Packet &p){
	if (getAvailableSpots() > 0){
		this->window[this->spots] = p;
		this->spots++;
	}
}

void Communicator::WindowManager::insert(std::vector<Packet> &vec){
	int n = vec.size();
	while(getAvailableSpots() > 0 && n > 0){
		this->window[this->spots] = vec.at(vec.size() - n);
		this->spots++;
		n--;
	}
	if (n > 0)
		throw std::exception("Not all element could be inserted");
}

void Communicator::WindowManager::ack(Packet &p){
	if (!p.isAck())
		throw std::exception("Packet is not an ACK");
	bool found = false;
	for (int i = 0; i < this->spots; i++){
		if (p.getId() == this->window[i].getId()){
			leftShift(i + 1);
			found = true;
		}
	}
	// NOTE: Ignore all ack that do not match any package in the window
	/*
	if (!found){
		std::stringstream ss;
		ss << "Packet not found"<< std::endl;
		ss << "WINDOW [ ";
		for(int i = 0; i < this->spots; i++)
			ss << this->window[i].getId() << ",";
		ss << "]" << std::endl;
		ss << "PACKET "<< p.getId();
		throw std::exception(ss.str().c_str());
	}
	*/
}

std::vector<Packet> Communicator::WindowManager::getPackets(){
	std::vector<Packet> packets;
	for (int i = 0; i < this->spots; i++)
		packets.push_back(this->window[i]);
	return packets;
}

void Communicator::WindowManager::leftShift(int shift){
	for (int i = shift; i < this->size; i++){
		this->window[i - shift] = this->window[i];
	}
	this->spots -= shift;
}