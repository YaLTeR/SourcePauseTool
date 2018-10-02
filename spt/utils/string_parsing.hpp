#pragma once
#include <string>

void ReplaceAll(std::string& str, const std::string& from, const std::string& to);
template<typename T>
void GetDoublet(std::string& s, T& out1, T& out2, char delim);
template<typename T>
void GetDoublet(std::istringstream& stream, T& out1, T& out2, char delim);
template<typename T>
void GetTriplet(std::string& s, T& out1, T& out2, T& out3, char delim);
template<typename T>
void GetTriplet(std::istringstream& stream, T& out1, T& out2, T& out3, char delim);
template<typename T>
T ParseValue(std::string s);
template<typename T>
bool IsValue(std::string s);

inline void GetDoublet(std::istringstream& stream, std::string& out1, std::string& out2, char delim)
{
	std::getline(stream, out1, delim);
	if (!stream.good())
		throw std::exception("Unable to read doublet!");
	std::getline(stream, out2, delim);
}

template<typename T>
inline void GetDoublet(std::string& s, T & out1, T & out2, char delim)
{
	std::istringstream is(s);
	GetDoublet(is, out1, out2, delim);
}

template<typename T>
inline void GetDoublet(std::istringstream& stream, T& out1, T& out2, char delim)
{
	std::string s1, s2;
	GetDoublet(stream, s1, s2, delim);
	out1 = ParseValue<T>(s1);
	out2 = ParseValue<T>(s2);
}

template<typename T>
inline void GetTriplet(std::string & s, T & out1, T & out2, T& out3, char delim)
{
	std::istringstream is(s);
	GetTriplet(is, out1, out2, out3, delim);
}

template<typename T>
inline void GetTriplet(std::istringstream & stream, T & out1, T & out2, T & out3, char delim)
{
	std::string s1, s2, s3;
	GetTriplet(stream, s1, s2, s3, delim);
	out1 = ParseValue<T>(s1);
	out2 = ParseValue<T>(s2);
	out3 = ParseValue<T>(s3);
}

inline void GetTriplet(std::istringstream& stream, std::string& out1, std::string& out2, std::string& out3, char delim)
{
	std::getline(stream, out1, delim);
	if (!stream.good())
		throw std::exception("Unable to read triplet!");
	std::getline(stream, out2, delim);
	if (!stream.good())
		throw std::exception("Unable to read triplet!");
	std::getline(stream, out3, delim);
}

template<typename T>
inline T ParseValue(std::string s)
{
	T result;
	std::stringstream ss(s);
	result = 0;
	ss >> result;

	if (ss.good() || s.size() == 0 || (result == 0 && s[0] != '0'))
		throw std::exception("Unable to parse value!");

	return result;
}

template<typename T>
inline bool IsValue(std::string s)
{
	std::stringstream ss(s);
	T result = 0;
	ss >> result;

	if (ss.good() || s.size() == 0 || (result == 0 && s[0] != '0'))
		return false;
	else
		return true;
}