#pragma once

#include <exception>

namespace core
{
	class FatalException : public std::exception
	{
	public:
		FatalException(std::string str) : exception(), str(str)
		{
	#ifdef _DEBUG
			if (IsDebuggerPresent()) _CrtDbgBreak();
	#endif
		}

		virtual const char *what() const
		{
			return str.c_str();
		}

	private:
		std::string str;
	};
}
