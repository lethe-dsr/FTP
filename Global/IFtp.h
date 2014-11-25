#pragma once
#define GET_COMMAND "GET"
#define PUT_COMMAND "PUT"

class IFtp {
public:
	virtual void doGet() = 0;
	virtual void doPut() = 0;
}