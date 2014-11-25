#pragma once
#include <string>

#define DELIMITER '|'

using std::string;

/*
Pracket provides methods to pack and unpack messages
*/
class Packet {
public:
	Packet();
	Packet(const char* buffer);
	Packet(int id);
	Packet(int id, const char *buffer, size_t length);
	Packet(const Packet& other);
	~Packet();
	Packet& operator=(const Packet& other);
	// Setters
	void setMessage(const char *buffer, size_t length);
	void setId(int id);
	// Getter
	string getMessageString() const;
	char* getMessage();
	size_t getMessageLength() const;
	int getId();
	// Buffers
	void decode(char *buffer, size_t length);
	size_t encode(char **buffer, size_t length);
	// Utilities
	bool isAck() const;
	static Packet makeAck(int id);

private:
	int id;
	char *message;
	size_t length;
	friend std::ostream& operator<< (std::ostream& os, const Packet &p);
};
