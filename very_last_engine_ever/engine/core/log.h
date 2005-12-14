#pragma once

namespace log {
	void clear(void);
	void printf(const char *format, ...);
	void save(std::string filename);
}
