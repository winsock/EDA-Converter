#ifndef __CONVERTER__
#define __CONVERTER__

#include <string>
#include <initializer_list>
#include <vector>
#include <stdexcept>

enum class eda_type {
    OPEN_JSON,
    EAGLE,
    KICAD,
    GEDA
};

enum class output_type {
    SCHEMATIC,
    LAYOUT,
    ALL
};

class converter {
private:
    std::vector<std::string> eda_files;
public:
    converter();
    bool openFiles(std::initializer_list<std::string> files);
    bool write(eda_type type);
};

class parse_exception : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class eda_format {
public:
    virtual void read(std::initializer_list<std::string> files) = 0;
    virtual void write(output_type type, std::string out_file) = 0;
};

#endif /* defined(__CONVERTER__) */