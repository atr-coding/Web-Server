#pragma once

#include <string>
#include <sstream>
#include <algorithm>
#include <map>

#define POST 0
#define GET 1

class HTTPRequest
{
public:
	HTTPRequest() = default;

	HTTPRequest(const std::string& request)
	{
		data = request;
		data.erase(std::remove(data.begin(), data.end(), '\r'), data.end());
		auto lines = explode(data, '\n');

		if (lines.size() > 0)
		{
			// Parse the first line to check and make sure the HTTP request is good, whether it is a POST or GET
			// and retrieve the given file/data if needed
			auto first_line = explode(lines.at(0), ' ');
			if (first_line.size() == 3)
			{
				if (first_line.at(2) == "HTTP/1.1")
				{
					if (first_line.at(0) == "POST") {
						type = POST;
					} else if (first_line.at(0) == "GET") {
						type = GET;
					} else {
						type = GET;
					}

					path = first_line.at(1);
					if (path.find("?")) {
						auto i = explode(path, '?');
						if (i.size() == 2) {
							parsePostData(i.at(1));
						}
					}
				}
			}

			// Parse the rest of the request
			for (auto l : lines) {
				l.erase(std::remove(l.begin(), l.end(), ' '), l.end());
				auto line = explode(l, ':');
				if (line.size() >= 2) {
					parseRequestLine(line);
				}
			}
		}
	}

	const std::string& getContent() const
	{
		return content;
	}

	const unsigned int getContentSize() const
	{
		return content.size();
	}

	const std::map<std::string, std::string>& getVariables() const
	{
		return variables;
	}

	const std::string& getPath() const
	{
		return path;
	}

	const int getType() const
	{
		return type;
	}
protected:
	void parseRequestLine(std::vector<std::string> line)
	{
		if (line.at(0) == "Content-Length")
		{
			unsigned int content_length = (unsigned int)std::stoi(line.at(1));
			content = data.substr(data.size() - content_length, content_length);
		}
	}

	void parsePostData(std::string data)
	{
		auto vars = explode(data, '&');
		for (auto var : vars)
		{
			auto v = explode(var, '=');
			if (v.size() == 2)
			{
				std::replace(v.at(1).begin(), v.at(1).end(), '+', ' ');
				variables.insert({ v.at(0), v.at(1) });
			}
		}
	}
private:
	int type				{ GET };
	std::string content		{ "" };
	std::string data		{ "" };
	std::string path		{ "" };
	std::map<std::string, std::string> variables;
};
