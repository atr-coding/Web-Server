#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <sstream>

inline std::vector<std::string> explode(std::string const& s, char delim)
{
	std::vector<std::string> result;
	std::istringstream iss(s);

	for (std::string token; std::getline(iss, token, delim); )
	{
		result.push_back(std::move(token));
	}

	return result;
}

inline std::string loadFile(const std::string& path, bool binary = false)
{
	std::ifstream file(path, (binary ? std::ios::binary : std::ios::in));
	if (file.is_open()) {
		std::stringstream ss;
		ss << file.rdbuf();
		return ss.str();
	}
	printf("Failed to load file.\n");
	return "";
}