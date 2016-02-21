#include <string>
#include <map>
#include <vector>
#include <memory>

namespace OpenJSON {
    namespace Types {
        typedef struct Point {
            int64_t x, y;
            Point() : Point(0, 0) {}
            Point(int xPos, int yPos) : x(xPos), y(yPos) {}
        } Point;
            
        namespace Shapes {
            enum class Alignment {
                LEFT, RIGHT, CENTER
            };
            
            enum class Type {
                RECTANGLE, ROUNDED_RECTANGLE, ARC, CIRCLE, LABEL, LINE, POLYGON, BEZIER_CURVE
            };
            
            // What is this used for?
            enum class Baseline {
                ALPHABET,
            };
            
            class Shape {
            public:
                Type type;
                std::map<std::string, std::string> styles;
            float rotation = 0.0f;
                bool flip = false;
            private:
            public:
                Shape (Type shapeType) : type(shapeType) {}
            };
                
            class Label : public Shape {
                Alignment align = Alignment::LEFT;
                Baseline baseline = Baseline::ALPHABET;
                std::string fontFamily;
                std::string text;
                int fontSize;
                Point position;
            public:
                Label() : Shape(Type::LABEL) {}
            };
            
            class Line : public Shape {
                unsigned int width = 10;
                Point start, end;
            public:
                Line() : Shape(Type::LINE) {}
            };
        };
        
        class Annotation {
            float rotation = 0.0f;
            Point position;
            bool flip = false, visible = true;
            Shapes::Label label;
        };
        
        class SymbolAttribute {
            float rotation = 0.0f;
            Point position;
            bool flip = false, hidden = false;
            std::vector<Annotation> annotations;
        };
        
        class ActionRegion {
            std::map<std::string, std::string> attributes;
            std::vector<int> connections;
            std::string name;
            Point p1, p2;
            std::string refID;
        };
        
        class Body {
            float rotation = 0.0f;
            std::vector<int> connections;
            bool flip = false, moveable = true, removable = true;
            std::string layerName;
            std::vector<Shapes::Shape> shapes;
            std::vector<ActionRegion> actionRegions;
            std::vector<Annotation> annotations;
            std::map<std::string, std::string> styles;
        };
        
        class GeneratedObject {
            std::map<std::string, std::string> attributes;
            std::vector<int> connections;
            std::string layerName;
            bool flip = false;
            float rotation = 0.0f;
            Point position;
        };
        
        class Symbol {
            std::vector<Body> bodies;
        };
        
        class Footprint {
            std::vector<Body> bodies;
            std::vector<GeneratedObject> generatedObjects;
        };
        
        class Component {
            std::string libraryID;
            std::map<std::string, std::string> attributes;

        };
        
        class ComponentInstance {
            Component componentDef;
            std::map<std::string, std::string> attributes;
            std::vector<SymbolAttribute> symbolAttributes;
            std::string instanceID;
            std::string symbolID;    
        };
        
        class NetPoint {
           typedef struct {
                unsigned int actionRegionIndex;
                unsigned int bodyIndex;
                std::string componentInstanceID;
                int orderIndex = 0;
                std::string signalName;
            } ConnectedActionRegion;
            
            std::string pointID;
            std::vector<ConnectedActionRegion> actionRegions;
            std::map<std::string, std::shared_ptr<NetPoint>> connectedPoints;
            Point position;
        };
        
        class Net {
            enum class Type {
                NETS
            };
            
            std::vector<Annotation> annotations;
            std::map<std::string, std::string> attributes;
            std::string netID;
            Type netType = Type::NETS;
            std::vector<NetPoint> points;
            std::vector<std::string> signals;
        };
        
        class Trace {
            enum class Type {
                STRAIGHT
            };
            
            std::string layerName;
            Point start, end;
            std::vector<Point> controlPoints;
            double width;
        };
        
        class Pour {
            std::string attachedNetID;
            std::map<std::string, std::string> attributes;
            std::string layerName;
            int orderIndex = 0;
            std::vector<Point> points;
            // TODO There is a polygon object that doesn't make sense to me
            Shapes::Type shapeTypes;
        };
        
        class PCBText {
            bool flip = false, visible = true;
            Shapes::Label label;
            std::string layerName;
            float rotation = 0.0f;
            std::string text;
            Point position;
        };
        
        class LayoutObject {
            std::map<std::string, std::string> attributes;
            std::vector<int> connections;
            bool flip = false;
            std::string layerName;
            float rotation = 0.0f;
            Point position;
        };
        
        class LayoutBodyAttribute {
            bool flip = false;
            std::string layerName;
            float rotation = 0.0f;
            Point position;
        };
        
        class LayerOption {
            std::string ident;
            bool isCopper = true;
            std::string name;
        };
        
        class DesignInfo {
            typedef struct {
                std::vector<std::string> attachedLinks;
                std::string description;
                std::string designID;
                std::string license;
                std::string name;
                std::string owner;
                std::string slug;
                uint64_t lastUpdatedTimestamp;
            } Metadata;
            
            std::vector<Annotation> annotations;
            std::map<std::string, std::string> attributes;
            Metadata metadata;
        };
    };
    
    class Data {
        typedef struct {
            std::string exporter, fileVersion;
        } Version;
        
        Version versionInfo;
        Types::DesignInfo designInfo;
        
        std::vector<Types::Component> components;
        std::vector<Types::ComponentInstance> componentInstances;
        std::vector<Types::LayerOption> layerOptions;
        std::vector<Types::Body> layoutBodies;
        std::vector<Types::LayoutBodyAttribute> layoutBodyAttributes;
        std::vector<Types::LayoutObject> layoutObjects;
        std::vector<Types::Net> nets;
        std::vector<Types::PCBText> pcbText;
        std::vector<Types::Pour> pours;
        std::vector<Types::Trace> traces;
    };
    
    class Reader {
        
    };
    
    class Writer {
        
    };
};