#include "PacketProvider.h"
#include <cstring>
#include <exception>
#include <sstream>

PacketProvider::PacketProvider(size_t chunk_size) :
chunk_size(chunk_size)
{}

FilePacketProvider::FilePacketProvider(const char *path, size_t chunk_size) :
PacketProvider(chunk_size),
read(0)
{
	this->is.open(path, std::ios::binary);
	if (is.fail()){
		std::stringstream ss;
		ss << "Input stream cannot be opened (" << path << ")";
		throw std::exception(ss.str().c_str());
		throw std::exception("Input stream cannot be opened");
	}

	size_t pos = is.tellg();
	is.seekg(0, std::ios::end);
	this->size = is.tellg();
	is.seekg(pos, std::ios::beg);
}

FilePacketProvider::~FilePacketProvider(){
	this->is.close();
}

vector<Packet> FilePacketProvider::get(int number){
	vector<Packet> vec;
	char *buffer = new char[this->chunk_size];
	for (int i = 0; i < number; i++){
		is.read(buffer, this->chunk_size);
		size_t char_read = is.gcount();
		this->read += char_read;
		if (char_read == 0) break;
		vec.push_back(Packet(0, buffer, char_read));
	}

	delete[] buffer;
	return vec;
}

bool FilePacketProvider::isCompleted(){
	return this->read == this->size && is.eof();
}