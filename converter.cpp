#include <iostream>
#include <cstdlib>
#include <iterator>

#include "converter.hpp"
#include "openjson.hpp"

int main(int argc, char** argv) {
    converter convert;
    // XXX REMOVE AND REPLACE WITH ACTUAL CLI THIS IS JUST TESTING
    if (convert.openFiles({"test_file.upv"}))
        return EXIT_SUCCESS;
    return EXIT_FAILURE;
}

converter::converter() {}

bool converter::openFiles(std::initializer_list<std::string> files) {
    std::copy(files.begin(), files.end(), std::back_inserter(this->eda_files));
    open_json::open_json_format parser;
    try {
        parser.read(files);
    } catch (parse_exception e) {
        std::cerr<<"Parse Error: "<<e.what()<<std::endl;
        return false;
    } catch (std::exception e) {
        std::cerr<<"Uncaught Exception: "<<e.what()<<std::endl;
        return false;
    }
    std::cout<<"Sucessfully read the input files!"<<std::endl;
    return true;
}