#pragma once
#include <iostream>
#include <tuple>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <iterator>
#include <utility>

class Exception : public std::exception {
public:
	Exception(const std::string_view& message, const std::string_view& function)
		: errorMessage(message), functionName(function) {}
	const char* what() const noexcept override {
		return errorMessage.c_str();
	}

	const char* where() const noexcept {
		return functionName.c_str();
	}

private:
	std::string errorMessage;
	std::string functionName;
};

// Create custom split() function.  
void customSplit(std::string str, char separator, std::vector<std::string>& strings) {
	int startIndex = 0, endIndex = 0;
	for (int i = 0; i <= str.size(); i++) {
		if (str[i] == separator || i == str.size()) {
			endIndex = i;
			std::string temp;
			temp.append(str, startIndex, endIndex - startIndex);
			strings.push_back(temp);
			startIndex = endIndex + 1;
		}
	}
}

// Recursively template function for reading values from a string
template <size_t Index, class... Types>
void readLineToTuple(const std::vector<std::string>& strings, std::tuple<Types...>& tuple) {
	std::stringstream ss;
	if (Index >= strings.size()) {
		throw Exception("Count of words in string larger then count of arguments in tuple", __func__);
	}
	ss << strings[Index];
	// Reading a value from a string into the appropriate type
	ss >> std::get<Index>(tuple);

	if constexpr (Index + 1 < sizeof...(Types)) {
		readLineToTuple<Index + 1>(strings, tuple);
	}
}

template<class... FieldTypes>
class CSVParser {
public:
	using TupleType = std::tuple<FieldTypes...>;
	CSVParser(std::ifstream& ifs, unsigned int count) : ifs(ifs), skipCount(count) {
		ifs.seekg(0, std::ios::end);
		endPos = 1 + ifs.tellg();
		ifs.clear();
		ifs.seekg(0);
	}

	class Iterator
	{
	public:
		Iterator(TupleType* point, std::ifstream& ifs, int curPos) : point(point), ifs(ifs), curPos(curPos) {}
		~Iterator() {
			delete point;
		}
		int operator++() {
			std::string line;
			if (std::getline(ifs, line, ';')) {
				std::vector<std::string> strings;
				customSplit(line, ',', strings);

				TupleType* tp = new TupleType;
				readLineToTuple<0>(strings, *tp);
				delete point;
				point = tp;
				curPos = ifs.tellg();
			}
			else {
				if (ifs.bad()) {
					int row = (curPos / 2 * sizeof...(FieldTypes)) + 1;
					int col = curPos % 2 * sizeof...(FieldTypes);
					std::string err = "I/O error while reading.File position : ";
					err += row + "row, ";
					err += col + "col";
					throw Exception(err, "file");
				}
				else if (ifs.eof()) {
					curPos++;
				}
				else if (ifs.fail()) {
					throw Exception("Wrong file format", "file");
				}
			}
			return curPos;
		}
		bool operator!=(const Iterator& endIt) {
			return curPos != endIt.curPos;
		}
		TupleType& operator*() {
			return *point;
		}
	private:
		TupleType* point;
		std::ifstream& ifs;
		int curPos;
	};
	
	Iterator begin() {
		std::string line;
		unsigned int count = 0;
		while (count < skipCount && std::getline(ifs, line, ';')) {
			count++;
		}
		if (std::getline(ifs, line, ';')) {
			std::vector<std::string> strings;
			customSplit(line, ',', strings);
			
			TupleType* tp = new TupleType;
			readLineToTuple<0>(strings, *tp);

			Iterator iter(tp, ifs, ifs.tellg());
			return iter;
		}
		else {
			int row = (ifs.tellg() / 2 * sizeof...(FieldTypes)) + 1;
			int col = ifs.tellg() % 2 * sizeof...(FieldTypes);
			std::string err = "";
			if (ifs.bad()) {				
				err = "I/O error while reading.File position : ";
			}
			else if (ifs.eof()) {
				err = "End of file reached";
			}
			else if (ifs.fail()) {
				err = "Wrong file format";
			}
			err += row + "row, ";
			err += col + "col";
			throw Exception(err, "file");
		}
	}
	Iterator end() {
		Iterator iter(nullptr, ifs, endPos);
		return iter;
	}
private:
	std::ifstream& ifs;
	unsigned int skipCount;
	int endPos;
};

template<typename TupleT, std::size_t... Is>
std::ostream& printTuple(std::ostream& os, const TupleT& tp, std::index_sequence<Is...>) {
	auto printElem = [&os](const auto& x, size_t index) {
		if (index > 0) {
			os << ',';
		}
		os << x;
		};
	os << '(';
	(printElem(std::get<Is>(tp), Is), ...);
	os << ')';
	return os;
}

template<typename TupleT, std::size_t TupleSize = std::tuple_size<TupleT>::value>
std::ostream& operator<<(std::ostream& os, const TupleT& tp) {
	return printTuple(os, tp, std::make_index_sequence<TupleSize>{});
}