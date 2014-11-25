#pragma once
#include "Packet.h"
#include <vector>
#include <fstream>


using std::vector;

/*
Packet factory used to split message into packets
*/
class PacketProvider {
public:
	PacketProvider(size_t chunk_size);
	virtual vector<Packet> get(int number) = 0;
	virtual bool isCompleted() = 0;
protected:
	size_t chunk_size;
};

/*
Packet factory from file
NOTE: Packet have all id 0
*/
class FilePacketProvider : public PacketProvider {
public:
	FilePacketProvider(const char *path, size_t chunk_size);
	virtual ~FilePacketProvider();
	virtual vector<Packet> get(int max_number);
	virtual bool isCompleted();
private:
	size_t size;
	std::ifstream is;
	size_t read;
};