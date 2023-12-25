#pragma once
#include <iostream>
#include <tuple>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
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

template <size_t Index, class... Types>
void readLineToTuple(size_t stringIndexForSplit, const std::string& str, char separator, std::tuple<Types...>& tuple) {
	std::stringstream ss;
	size_t startIndex = stringIndexForSplit, endIndex = stringIndexForSplit;
	for (size_t i = stringIndexForSplit; i <= str.size(); i++) {
		if (str[i] == separator || i == str.size()) {
			endIndex = i;
			std::string tmp = str.substr(startIndex, endIndex - startIndex);
			ss << tmp;
			ss >> std::get<Index>(tuple);
			break;
		}
	}
	if constexpr (Index + 1 < sizeof...(Types)) {
		readLineToTuple<Index + 1>(endIndex + 1, str, separator, tuple);
	}
}

template<class... FieldTypes>
class CSVParser {
public:
	using TupleType = std::tuple<FieldTypes...>;
	CSVParser(std::ifstream& ifs, unsigned int count, char wordSeparator = ',', char stringSeparator = '\n') : ifs(ifs), 
		skipCount(count), wordSeparator(wordSeparator), stringSeparator(stringSeparator) {
		ifs.seekg(0, std::ios::end);
		endPos = 1 + ifs.tellg();
		ifs.clear();
		ifs.seekg(0);
	}

	class Iterator
	{
	public:
		Iterator(std::unique_ptr<TupleType> point, std::ifstream& ifs, size_t curPos, char wordSeparator, char stringSeparator) : point(std::move(point)), 
			ifs(ifs), curPos(curPos), wordSeparator(wordSeparator), stringSeparator(stringSeparator) {}
		size_t operator++() {
			std::string line;
			if (std::getline(ifs, line, stringSeparator)) {
				std::unique_ptr<TupleType> tp = std::make_unique<TupleType>();
				readLineToTuple<0>(0, line, wordSeparator, *tp);
				point = std::move(tp);
				curPos = ifs.tellg();
			}
			else {
				if (ifs.bad()) {
					size_t row = (curPos / 2 * sizeof...(FieldTypes)) + 1;
					size_t col = curPos % 2 * sizeof...(FieldTypes);
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
		std::unique_ptr<TupleType> point;
		std::ifstream& ifs;
		size_t curPos;
		char wordSeparator;
		char stringSeparator;
	};
	
	Iterator begin() {
		std::string line;
		unsigned int count = 0;
		while (count < skipCount && std::getline(ifs, line, stringSeparator)) {
			count++;
		}
		if (std::getline(ifs, line, stringSeparator)) {
			auto tp = std::make_unique<TupleType>();
			readLineToTuple<0>(0, line, wordSeparator, *tp);

			Iterator iter(std::move(tp), ifs, ifs.tellg(), wordSeparator, stringSeparator);
			return iter;
		}
		else {
			size_t row = (ifs.tellg() / 2 * sizeof...(FieldTypes)) + 1;
			size_t col = ifs.tellg() % 2 * sizeof...(FieldTypes);
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
		Iterator iter(nullptr, ifs, endPos, wordSeparator, stringSeparator);
		return iter;
	}
private:
	std::ifstream& ifs;
	unsigned int skipCount;
	size_t endPos;
	char wordSeparator;
	char stringSeparator;
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