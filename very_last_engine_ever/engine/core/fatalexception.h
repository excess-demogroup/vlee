#pragma once

#include <exception>

class FatalException : public std::exception {
public:
	FatalException(const char *str) : exception(), str(str) {}
	FatalException(std::string str) : exception(), str(str) {}

	virtual const char *what() const {
		return str.c_str();
	}

private:
	std::string str;
};
