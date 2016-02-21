#include <string>
#include <initializer_list>
#include <vector>
#include <stdexcept>

enum class EDAType {
    OPEN_JSON,
    EAGLE,
    KICAD,
    GEDA
};

class Converter {
private:
    std::vector<std::string> edaFiles;
    bool parse();
public:
    Converter();
    bool openFiles(std::initializer_list<std::string> files);
    bool write(EDAType type);
};

class parse_exception : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};