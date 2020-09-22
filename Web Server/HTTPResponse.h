#pragma once

#include <string>
#include <sstream>

class HTTPResponse
{
public:
	HTTPResponse() = default;

	HTTPResponse(const std::string& code)
	{
		ss << code << "\n";
	}

	void setContent(const std::string& _content, const std::string& type)
	{
		content = _content;
		ss << "Content-Type: " << type << "\n";
		ss << "Content-Length: " << content.size() << "\n";
	}

	void keepConnectionAlive(unsigned int timeout, unsigned int max)
	{
		ss << "Connection: Keep-Alive\n";
		ss << "Keep-Alive: timeout=" << timeout << ", max=" << max << "\n";
	}

	void compile()
	{
		ss << "\n" << content << "\n";
		result = ss.str();
	}

	const std::string& get() const
	{
		return result;
	}
private:
	std::stringstream ss;
	std::string content	{ "" };
	std::string result	{ "" };
};
