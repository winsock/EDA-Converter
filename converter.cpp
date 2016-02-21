#include <iostream>
#include <cstdlib>
#include <iterator>

#include "converter.hpp"

int main(int argc, char** argv) {
    std::cout<<"Hello World"<<std::endl;
    return EXIT_SUCCESS;
}

Converter::Converter() {
    
}

bool Converter::openFiles(std::initializer_list<std::string> files) {
    std::copy(files.begin(), files.end(), std::back_inserter(this->edaFiles));
    return this->parse();
}

bool Converter::parse() {
    for (std::string file : this->edaFiles) {
        
    }
}