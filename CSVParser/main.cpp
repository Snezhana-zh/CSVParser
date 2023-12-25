#include  "CSVParserHeader.h"

int main() {
    std::ifstream file("test.txt");
    CSVParser<int, int, std::string> parser(file, 0 /*skip first lines count*/);
    for (std::tuple<int, int, std::string> rs : parser) {
        std::cout << rs << std::endl;
    }
}