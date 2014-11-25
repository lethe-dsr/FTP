#pragma once
#pragma comment(lib, "Ws2_32.lib")
#include <WinSock2.h>
#include <string>
#include <vector>
#include "Packet.h"

#define MAX_BUFFER_SIZE 512
#define MAX_PACKET_SIZE 256
#define WINDOW_SIZE 8
#define MAX_INDEX   128
#define MAX_ATTEMPTS 5
#define TIMEOUT 2000 // in millisecond

using std::string;

/*
Communicator provides a base for a communication protocol
*/
class Communicator
{
public:

	Communicator(int port);
	~Communicator();

	// Hand shake
	bool init(sockaddr_in *destination = NULL);

	// Transmission
	bool   sendPacket(Packet &p, int attempts=MAX_ATTEMPTS);
	Packet recvPacket(int timeout_ms = TIMEOUT);

	void sendFile(string pathname, int attempts = MAX_ATTEMPTS);
	void recvFile(string pathname, int timeout_ms = TIMEOUT);

	// Data
	int getLocalIndex();
	int getRemoteIndex();

private:
	/*
	Manager for the window packet list
	*/
	class WindowManager {
	public:
		WindowManager(int window_size);
		~WindowManager();

		bool isEmpty();
		int getAvailableSpots();

		void insert(Packet &p);
		void insert(std::vector<Packet> &vec);
		void ack(Packet &p);
		std::vector<Packet> getPackets();
	private:
		void leftShift(int pos);
		Packet *window;
		int size;
		int spots;
	};

	// Hand shake
	void initiateHandShake(sockaddr_in *destination);
	void respondHandShake();
	// Index
	void increaseIndex();
	void increaseRemoteIndex();
	// Unreliable one packet message
	void sendto(sockaddr_in *destination, Packet& p);
	Packet recvfrom(TIMEVAL *timeout = NULL);
	// Timing
	TIMEVAL getTimeout(int millisecond = 0);

	SOCKET socket;
	long   index;
	long   remote_index;
	sockaddr_in local_address;
	sockaddr_in remote_address;
};

