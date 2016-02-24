#include <iostream>
#include <cstdlib>
#include <iterator>

#include "converter.hpp"
#include "openjson.hpp"

int main(int argc, char** argv) {
    converter convert;
    if (argc < 1) {
        return EXIT_FAILURE;
    }
    // TODO Make more robust!
    if (convert.openFiles(std::vector<std::string>(argv + 1, argv + argc)))
        return EXIT_SUCCESS;
    return EXIT_FAILURE;
}

converter::converter() {}

bool converter::openFiles(std::vector<std::string> files) {
    open_json::open_json_format parser;
    try {
        parser.read(files);
    } catch (parse_exception e) {
        std::cerr<<"Parse Error: "<<e.what()<<std::endl;
        return false;
    }
    std::cout<<"Sucessfully read the input files!"<<std::endl;
    // XXX REMOVE AFTER TESTING!
    try {
        parser.write(output_type::ALL, "_output.upv");
    } catch (std::exception e) {
        std::cerr<<"Write Error: "<<e.what()<<std::endl;
        return false;
    }
    std::cout<<"Sucessfully wrote to the output file!"<<std::endl;
    return true;
}