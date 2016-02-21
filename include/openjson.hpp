#include <string>
#include <regex>
#include <map>
#include <vector>
#include <memory>

#include "json.hpp"

using json = nlohmann::json;

namespace open_json {
    static std::vector<std::string> split(const std::string &input, const std::string &regex) {
        // passing -1 as the submatch index parameter performs splitting
        std::regex re(regex);
        std::sregex_token_iterator
            first{input.begin(), input.end(), re, -1},
            last;
        return {first, last};
    }
    
    namespace types {
        typedef struct point {
            int64_t x, y;
            point() : point(0, 0) {}
            point(int x_pos, int y_pos) : x(x_pos), y(y_pos) {}
        } point;
        
        class json_object {
        public:
            virtual void read(json json) = 0;
            virtual void write(json json) = 0;
        };
        
        namespace shapes {
            enum class alignment {
                LEFT, RIGHT, CENTER
            };
            
            enum class shape_type {
                RECTANGLE, ROUNDED_RECTANGLE, ARC, CIRCLE, LABEL, LINE, POLYGON, BEZIER_CURVE
            };
            
            // What is this used for?
            enum class baseline {
                ALPHABET,
            };
            
            class shape : public json_object {
            public:
                shape_type type;
                std::map<std::string, std::string> styles;
                float rotation = 0.0f;
                bool flip = false;

            public:
                shape (shape_type shape_type) : type(shape_type) {}
                virtual void read(json json) override;
                virtual void write(json json) override;
            };
                
            class label : public shape {
                alignment align = alignment::LEFT;
                baseline baseline = baseline::ALPHABET;
                std::string font_family;
                std::string text;
                int font_size;
                point position;
            public:
                label() : shape(shape_type::LABEL) {}
                void read(json json) override;
                void write(json json) override;
            };
            
            class line : public shape {
                unsigned int width = 10;
                point start, end;
            public:
                line() : shape(shape_type::LINE) {}
                void read(json json) override;
                void write(json json) override;
            };
        };
        
        class annotation : public json_object {
            float rotation = 0.0f;
            point position;
            bool flip = false, visible = true;
            shapes::label label;
        public:
            void read(json json) override;
            void write(json json) override;
        };
        
        class symbol_attribute : public json_object {
            float rotation = 0.0f;
            point position;
            bool flip = false, hidden = false;
            std::vector<annotation> annotations;
        public:
            void read(json json) override;
            void write(json json) override;
        };
        
        class action_region : public json_object {
            std::map<std::string, std::string> attributes;
            std::vector<int> connections;
            std::string name;
            point p1, p2;
            std::string ref_id;
        public:
            void read(json json) override;
            void write(json json) override;
        };
        
        class body : public json_object {
            float rotation = 0.0f;
            std::vector<int> connections;
            bool flip = false, moveable = true, removable = true;
            std::string layer_name;
            std::vector<shapes::shape> shapes;
            std::vector<action_region> action_regions;
            std::vector<annotation> annotations;
            std::map<std::string, std::string> styles;
        public:
            void read(json json) override;
            void write(json json) override;
        };
        
        class generated_object : public json_object {
            std::map<std::string, std::string> attributes;
            std::vector<int> connections;
            std::string layer_name;
            bool flip = false;
            float rotation = 0.0f;
            point position;
        public:
            void read(json json) override;
            void write(json json) override;
        };
        
        class symbol : public json_object {
            std::vector<body> bodies;
        public:
            void read(json json) override;
            void write(json json) override;
        };
        
        class footprint : public json_object {
            std::vector<body> bodies;
            std::vector<generated_object> generated_objects;
        public:
            void read(json json) override;
            void write(json json) override;
        };
        
        class component : public json_object {
            std::string library_id;
            std::map<std::string, std::string> attributes;
            std::vector<footprint> footprints;
            std::vector<symbol> symbols;
        public:
            component(std::string id) : library_id(id) {}
            void read(json json) override;
            void write(json json) override;
        };
        
        class component_instance : public json_object {
            std::shared_ptr<component> component_def;
            std::map<std::string, std::string> attributes;
            std::vector<symbol_attribute> symbol_attributes;
            std::string instance_id;
            std::string symbol_id;
        public:
            component_instance(std::shared_ptr<component> def) : component_def(def) {}
            void read(json json) override;
            void write(json json) override;
            std::string get_id() { return instance_id; }
        };
        
        class net_point : public json_object {
           typedef struct {
                unsigned int action_region_index;
                unsigned int body_index;
                std::string component_instance_id;
                int order_index = 0;
                std::string signal_name;
            } connected_action_region;
            
            std::string point_id;
            std::vector<connected_action_region> action_regions;
            std::map<std::string, std::shared_ptr<net_point>> connected_points;
            point position;
        public:
            void read(json json) override;
            void write(json json) override;
        };
        
        class net : public json_object {
            enum class type {
                NETS
            };
            
            std::vector<annotation> annotations;
            std::map<std::string, std::string> attributes;
            std::string net_id;
            type net_type = type::NETS;
            std::vector<net_point> points;
        public:
            void read(json json) override;
            void write(json json) override;
            std::string get_id() { return net_id; }
        };
        
        class trace : public json_object {
            enum class type {
                STRAIGHT
            };
            
            std::string layer_name;
            point start, end;
            std::vector<point> control_points;
            double width;
        public:
            void read(json json) override;
            void write(json json) override;
        };
        
        class pour : public json_object {
            std::string attached_net_id;
            std::map<std::string, std::string> attributes;
            std::string layer_name;
            int order_index = 0;
            std::vector<point> points;
            // TODO There is a polygon object that doesn't make sense to me
            shapes::shape_type shape_types;
        public:
            void read(json json) override;
            void write(json json) override;
        };
        
        class pcb_text : public json_object {
            bool flip = false, visible = true;
            shapes::label label;
            std::string layer_name;
            float rotation = 0.0f;
            std::string text;
            point position;
        public:
            void read(json json) override;
            void write(json json) override;
        };
        
        class layout_object : public json_object {
            std::map<std::string, std::string> attributes;
            std::vector<int> connections;
            bool flip = false;
            std::string layer_name;
            float rotation = 0.0f;
            point position;
        public:
            void read(json json) override;
            void write(json json) override;
        };
        
        class layout_body_attribute : public json_object {
            bool flip = false;
            std::string layer_name;
            float rotation = 0.0f;
            point position;
        public:
            void read(json json) override;
            void write(json json) override;
        };
        
        class layer_option : public json_object {
            std::string ident;
            bool is_copper = true;
            std::string name;
        public:
            void read(json json) override;
            void write(json json) override;
        };
        
        class design_info : public json_object {
            typedef struct {
                std::vector<std::string> attached_links;
                std::string description;
                std::string designID;
                std::string license;
                std::string name;
                std::string owner;
                std::string slug;
                uint64_t last_updated;
            } metadata_container;
            
            std::vector<annotation> annotations;
            std::map<std::string, std::string> attributes;
            metadata_container metadata;
        public:
            void read(json json) override;
            void write(json json) override;
        };
    };
    
    class data : public types::json_object {
        typedef struct {
            std::string exporter;
            int major = 0, minor = 2, build = 0;
        } version;
        
        version version_info;
        types::design_info design_info;
        
        std::map<std::string, std::shared_ptr<types::component>> components;
        std::map<std::string, std::shared_ptr<types::component_instance>> component_instances;
        std::map<std::string, std::shared_ptr<types::net>> nets;
        std::vector<std::shared_ptr<types::layer_option>> layer_options;
        std::vector<std::shared_ptr<types::body>> layout_bodies;
        std::vector<std::shared_ptr<types::layout_body_attribute>> layout_body_attributes;
        std::vector<std::shared_ptr<types::layout_object>> layout_objects;
        std::vector<std::shared_ptr<types::pcb_text>> pcb_text;
        std::vector<std::shared_ptr<types::pour>> pours;
        std::vector<std::shared_ptr<types::trace>> traces;
    public:
        void read(json json) override;
        void write(json json) override;
    };
};