#include "Packet.h"
#include <sstream>
#include <cstring>

Packet::Packet() : id(0), message(NULL), length(0){}

Packet::Packet(const char* buffer) : id(0), message(NULL), length(0){
	setMessage(buffer, std::strlen(buffer));
}

Packet::Packet(int id) : id(id), message(NULL), length(0){}

Packet::Packet(int id, const char *buffer, size_t length) : id(id), message(NULL), length(length){
	this->setMessage(buffer, length);
}

Packet::Packet(const Packet& other){
	this->id = other.id;
	this->length = other.length;
	if (this->length > 0){
		this->message = new char[this->length];
		std::memcpy(this->message, other.message, this->length);
	}
}

Packet::~Packet(){ if (message != NULL) delete[] message; }

Packet& Packet::operator=(const Packet& other){
	this->id = other.id;
	this->length = other.length;
	if (this->length > 0){
		this->message = new char[this->length];
		std::memcpy(this->message, other.message, this->length);
	}
	return *this;
}

void Packet::setMessage(const char *buffer, size_t length){
	if (message != NULL)
		delete[] this->message;
	this->length = length;
	this->message = new char[length];
	std::memcpy(this->message, buffer, length);
}

void Packet::setId(int id){ this->id = id; }

string Packet::getMessageString()  const{
	size_t size = this->length + 1;
	char *buffer = new char[size];
	std::memset(buffer, 0, size);
	std::memcpy(buffer, this->message, this->length);
	string s(buffer);
	delete[] buffer;
	return s;
}

char* Packet::getMessage(){ return this->message; }

size_t Packet::getMessageLength() const{ return this->length; }

int Packet::getId(){ return this->id; }

void Packet::decode(char *buffer, size_t length){
	char *cursor = NULL;
	this->id = std::strtol(buffer, &cursor, 10);
	cursor += 1; // skipping the delimiter
	if (this->message != NULL)
		delete[] message;
	this->length = length - (cursor - buffer);
	this->message = new char[this->length];
	std::memcpy(this->message, cursor, this->length);
}

size_t Packet::encode(char **buffer, size_t length){
	std::stringstream ss;
	ss << this->id << DELIMITER;
	string meta_data = ss.str();
	size_t buffer_size = -1;
	if (meta_data.size() < length){
		std::memcpy(*buffer, meta_data.c_str(), meta_data.size());
		length -= meta_data.size();
		if (length < this->length){
			std::memcpy(*buffer, this->message, length);
			buffer_size = length + meta_data.size();
		}
		else{
			std::memcpy(*buffer + meta_data.size(), this->message, this->length);
			buffer_size = this->length + meta_data.size();
		}
	}
	return buffer_size;
}

bool Packet::isAck() const{
	if (this->length == 3)
		return this->message[0] == 'A' && this->message[1] == 'C' && this->message[2] == 'K';
	return false;
}

Packet Packet::makeAck(int id){
	Packet p(id);
	p.setMessage("ACK", 3);
	return p;
}

std::ostream& operator<<(std::ostream &os, const Packet &p){
	os << "PACKET " << p.id << ": { length=" << p.length << "\"" << p.getMessageString() << "\" }";
	return os;
}
