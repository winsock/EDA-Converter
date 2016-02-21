#include <string>
#include <initializer_list>
#include <vector>

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