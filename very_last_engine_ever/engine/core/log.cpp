#include "stdafx.h"
#include "../engine.h"

using namespace engine::core;

std::ostrstream logstream;

void log::clear(void) {
	logstream.clear();
}

void log::printf(const char *format, ...) {
	static char buffer[1024];
	va_list arguments;

	if (!format) return;
	va_start(arguments, format);
	vsprintf_s(buffer, 1024, format, arguments);
	va_end(arguments);

	logstream << buffer;
}

void log::save(std::string filename) {
//	FILE *fp = fopen_s(filename.c_str(), "w");
	FILE *fp;
	fopen_s(&fp, filename.c_str(), "w");

	if (!fp) throw FatalException("unable to open logfile!");

	char *string = new char[logstream.pcount() + 1];
	memcpy(string, logstream.str(), logstream.pcount() + 1);
	string[logstream.pcount()] = '\0';
	fprintf(fp, "%s", string);
	delete string;

	fclose(fp);
}
