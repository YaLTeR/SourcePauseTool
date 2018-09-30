#pragma once
#include <random>
#include <sstream>

namespace scripts
{
	class SearchDoneException : public std::exception
	{};

	enum SearchResult { Success, Fail, Low, High };
	enum SearchType { None, Lowest, Range, Highest, Random };

	template <typename T>
	class RangeVariable
	{
	public:
		RangeVariable();
		void Select(SearchResult lastResult, SearchType type);
		std::string GetValue();
		std::string GetRangeString();
		void ParseInput(std::string value, bool angle);
		void ParseValues(std::string lowS, std::string highS, std::string incS);

	private:
		void SelectLow(SearchResult lastResult);
		void SelectHigh(SearchResult lastResult);
		void SelectRandom(SearchResult lastResult);
		T Normalize(T value);
		T GetValueForIndex(int index);

		bool isAngle;
		T initialLow;
		T initialHigh;
		int lowIndex;
		int valueIndex;
		int highIndex;
		T increment;
		int lastIndex;

		std::mt19937 rng;
		std::uniform_int_distribution<int> uniformRandom;
	};

	template<typename T>
	inline T RangeVariable<T>::GetValueForIndex(int index)
	{
		auto value = initialLow + index * increment;
		return Normalize(value);
	}

	template<typename T>
	inline std::string RangeVariable<T>::GetValue()
	{
		return std::to_string(GetValueForIndex(valueIndex));
	}

	template<typename T>
	inline std::string RangeVariable<T>::GetRangeString()
	{
		std::ostringstream os;

		os << "[low: " << GetValueForIndex(lowIndex)
			<< ", value: " << GetValueForIndex(valueIndex)
			<< ", high: " << GetValueForIndex(highIndex) << "]";

		return os.str();
	}

	template<typename T>
	inline void RangeVariable<T>::ParseInput(std::string value, bool angle)
	{
		isAngle = angle;
		std::istringstream is(value);
		std::string lowS, highS, incS;

		std::getline(is, lowS, '|');
		std::getline(is, highS, '|');
		std::getline(is, incS, '|');

		ParseValues(lowS, highS, incS);
		uniformRandom = std::uniform_int_distribution<int>(lowIndex, highIndex);
	}


	inline void RangeVariable<int>::ParseValues(std::string lowS, std::string highS, std::string incS)
	{
		initialLow = std::stoi(lowS);
		initialHigh = std::stoi(highS);
		increment = std::stoi(incS);

		lowIndex = 0;
		highIndex = (initialHigh - initialLow) / increment;
	}

	inline void RangeVariable<float>::ParseValues(std::string lowS, std::string highS, std::string incS)
	{
		initialLow = std::stof(lowS);
		initialHigh = std::stof(highS);
		increment = std::stof(incS);

		lowIndex = 0;
		highIndex = static_cast<int>((initialHigh - initialLow) / increment);
	}

	template<typename T>
	inline RangeVariable<T>::RangeVariable()
	{
		std::random_device rd;
		rng = std::mt19937(rd());
	}

	template<typename T>
	inline void RangeVariable<T>::Select(SearchResult lastResult, SearchType type)
	{
		switch (type)
		{
		case Lowest:
			SelectLow(lastResult);
			break;
		case Highest:
			SelectHigh(lastResult);
			break;
		case Random:
			SelectRandom(lastResult);
			break;
		case Range:
			throw std::exception("Search type Range not implemented.");
			break;
		}

		// Search stuck in place, stop searching
		if (type != Random && lastIndex == valueIndex)
		{
			throw SearchDoneException();
		}

		lastIndex = valueIndex;
	}

	template<typename T>
	inline void RangeVariable<T>::SelectLow(SearchResult lastResult)
	{
		switch (lastResult)
		{
		case Low:
			lowIndex = valueIndex;
			valueIndex = (lowIndex + highIndex) / 2 + 1;
			break;
		case Success: case High:
			highIndex = valueIndex;
			valueIndex = (lowIndex + highIndex) / 2 + 1;
			break;
		default:
			throw std::exception("Unexpected search result received. Lowest search only accepts Low, High and Success");
		}

	}

	template<typename T>
	inline void RangeVariable<T>::SelectHigh(SearchResult lastResult)
	{
		switch (lastResult)
		{
		case Success: case Low:
			lowIndex = valueIndex;
			valueIndex = (lowIndex + highIndex) / 2 + 1;
			break;
		case High:
			highIndex = valueIndex;
			valueIndex = (lowIndex + highIndex) / 2 + 1;
			break;
		default:
			throw std::exception("Unexpected search result received. Highest search only accepts Low, High and Success");
		}

	}

	template<typename T>
	inline void RangeVariable<T>::SelectRandom(SearchResult lastResult)
	{
		valueIndex = uniformRandom(rng);
	}

	inline float NormalizeDeg(double a)
	{
		a = std::fmod(a, 360.0);
		if (a >= 180.0)
			a -= 360.0;
		else if (a < -180.0)
			a += 360.0;
		return static_cast<float>(a);
	}

	template<typename T>
	inline T RangeVariable<T>::Normalize(T value)
	{
		return value;
	}

	inline float RangeVariable<float>::Normalize(float value)
	{
		if (isAngle)
			value = NormalizeDeg(static_cast<double>(value));

		return value;
	}
}