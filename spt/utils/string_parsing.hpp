#pragma once
#include <string>

std::wstring s2ws(const std::string& s);
bool whiteSpacesOnly(const std::string& s);

void ReplaceAll(std::string& str, const std::string& from, const std::string& to);
inline void GetTriplet(std::istringstream& stream, std::string& out1, std::string& out2, std::string& out3, char delim);
template<typename T>
inline T ParseValue(std::string s);
template<typename T>
inline bool IsValue(std::string s);

inline void GetDoublet(std::istringstream& stream, std::string& out1, std::string& out2, char delim)
{
	std::getline(stream, out1, delim);
	if (!stream.good())
		throw std::exception("Unable to read doublet!");
	std::getline(stream, out2);
}

template<typename T1, typename T2>
inline void GetDoublet(const std::string& s, T1& out1, T2& out2, char delim)
{
	std::istringstream is(s);
	GetDoublet(is, out1, out2, delim);
}

template<typename T1, typename T2>
inline void GetDoublet(std::istringstream& stream, T1& out1, T2& out2, char delim)
{
	std::string s1, s2;
	GetDoublet(stream, s1, s2, delim);
	out1 = ParseValue<T1>(s1);
	out2 = ParseValue<T2>(s2);
}

template<typename T1, typename T2, typename T3>
inline void GetTriplet(const std::string& s, T1& out1, T2& out2, T3& out3, char delim)
{
	std::istringstream is(s);
	GetTriplet(is, out1, out2, out3, delim);
}

inline void GetStringTriplet(const std::string& s, std::string& out1, std::string& out2, std::string& out3, char delim)
{
	std::istringstream is(s);
	GetTriplet(is, out1, out2, out3, delim);
}

template<typename T1, typename T2, typename T3>
inline void GetTriplet(std::istringstream& stream, T1& out1, T2& out2, T3& out3, char delim)
{
	std::string s1, s2, s3;
	GetTriplet(stream, s1, s2, s3, delim);
	out1 = ParseValue<T1>(s1);
	out2 = ParseValue<T2>(s2);
	out3 = ParseValue<T3>(s3);
}

template<typename T1, typename T2, typename T3, typename T4>
inline void GetQuadlet(const std::string& s, T1& out1, T2& out2, T3& out3, T4& out4, char delim)
{
	std::istringstream is(s);
	GetQuadlet(is, out1, out2, out3, out4, delim);
}

template<typename T1, typename T2, typename T3, typename T4>
inline void GetQuadlet(std::istringstream& stream, T1& out1, T2& out2, T3& out3, T4& out4, char delim)
{
	std::string s1, s2, s3, s4;
	GetQuadlet(stream, s1, s2, s3, s4, delim);
	out1 = ParseValue<T1>(s1);
	out2 = ParseValue<T2>(s2);
	out3 = ParseValue<T3>(s3);
	out4 = ParseValue<T4>(s4);
}

inline void GetTriplet(std::istringstream& stream, std::string& out1, std::string& out2, std::string& out3, char delim)
{
	std::getline(stream, out1, delim);
	if (!stream.good())
		throw std::exception("Unable to read triplet!");
	std::getline(stream, out2, delim);
	if (!stream.good())
		throw std::exception("Unable to read triplet!");
	std::getline(stream, out3);
}

inline void GetQuadlet(std::istringstream& stream, std::string& out1, std::string& out2, std::string& out3, std::string& out4, char delim)
{
	std::getline(stream, out1, delim);
	if (!stream.good())
		throw std::exception("Unable to read quadlet!");
	std::getline(stream, out2, delim);
	if (!stream.good())
		throw std::exception("Unable to read quadlet!");
	std::getline(stream, out3, delim);
	if (!stream.good())
		throw std::exception("Unable to read quadlet!");
	std::getline(stream, out4);
}

template<typename T>
inline T ParseValue(std::string s)
{
	T result = 0;
	std::stringstream ss(s);
	bool success = bool(ss >> result);

	if (!success)
	{
		std::ostringstream oss;
		oss << "Unable to parse value " << s << " \n";
		throw std::exception(oss.str().c_str());
	}

	return result;
}

template<typename T>
inline bool IsValue(std::string s)
{
	std::stringstream ss(s);
	T result = 0;
	return bool(ss >> result);
}