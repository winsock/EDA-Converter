#include <string>
#include <regex>
#include <map>
#include <unordered_map>
#include <vector>
#include <memory>
#include <algorithm>
#include <locale>
#include <functional>

#include "json.hpp"
#include "converter.hpp"

using json = nlohmann::json;

namespace open_json {
    inline std::vector<std::string> split(const std::string &input, const std::string &regex) {
        // passing -1 as the submatch index parameter performs splitting
        std::regex re(regex);
        std::sregex_token_iterator
            first{input.begin(), input.end(), re, -1},
            last;
        return {first, last};
    }
    
    inline bool get_boolean(json json_data, bool default_value = false) {
        bool value = default_value;
        if (json_data.is_null()) {
            return value;
        }
        
        if (json_data.is_boolean()) {
            value = json_data;
        } else if (json_data.is_string()) {
            std::string string_value = json_data;
            std::transform(string_value.begin(), string_value.end(), string_value.begin(), std::bind(&std::tolower<decltype(string_value)::value_type>, std::placeholders::_1, std::locale("")));
            if (string_value == "true") {
                value = true;
            }
        }
        return value;
    }
    
    namespace types {
        inline void populate_attributes(std::map<std::string, std::string> &attribute_map, json json_data) {
            for (json::iterator it = json_data.begin(); it != json_data.end(); it++) {
                attribute_map[it.key()] = it.value();
            }
        }
        
        typedef struct point {
            int64_t x, y;
            point() : point(0, 0) {}
            point(int x_pos, int y_pos) : x(x_pos), y(y_pos) {}
        } point;
        
        class json_object {
        public:
            virtual void read(json json_data) = 0;
            virtual json::object_t get_json() = 0;
        };
        
        namespace shapes {
            enum class shape_type {
                RECTANGLE, ROUNDED_RECTANGLE, ARC, CIRCLE, LABEL, LINE, ROUNDED_SEGMENT, POLYGON, BEZIER_CURVE
            };
            
            class shape;
            typedef std::shared_ptr<shape> (*create_function)(json json_data);
            typedef std::map<shape_type, create_function> shape_registry_type;
            extern shape_registry_type shape_registry;
            extern std::map<std::string, shape_type> shape_typename_registry;
            extern std::map<shape_type, std::string> name_shape_type_registry;
            
            class shape : public json_object {
            public:
                shape_type type;
                std::map<std::string, std::string> styles;
                float rotation = 0.0f;
                bool flip = false;
            public:
                shape (json json_data, shape_type shape_type) : type(shape_type) { this->read(json_data); }
                virtual void read(json json_data) override;
                virtual json::object_t get_json() override;
                static std::shared_ptr<shape> new_shape(shape_type type, json json_data) {
                    if (shape_registry.find(type) == shape_registry.end()) {
                        return nullptr;
                    }
                    return shape_registry[type](json_data);
                }
            };
            
            class rectangle : public shape {
                unsigned int line_width = 0;
                int width = 0, height = 0;
                point position;
            protected:
                rectangle(json json_data, shape_type type) : shape(json_data, type) { this->read(json_data); }
            public:
                rectangle(json json_data) : rectangle(json_data, shape_type::RECTANGLE) {}
                virtual void read(json json_data) override;
                virtual json::object_t get_json() override;
            };
            
            class rounded_rectangle : public rectangle {
                int radius = 3; // Corner rounding radius
            public:
                rounded_rectangle(json json_data) : rectangle(json_data, shape_type::ROUNDED_RECTANGLE) { this->read(json_data); }
                void read(json json_data) override;
                json::object_t get_json() override;
            };
            
            class arc : public shape {
                float start_angle = 0.0f, end_angle = 0.0f;
                int radius = 3;
                bool is_clockwise = true;
                unsigned int width = 0;
                point position;
            public:
                arc(json json_data) : shape(json_data, shape_type::ARC) { this->read(json_data); }
                void read(json json_data) override;
                json::object_t get_json() override;
            };
            
            class circle : public shape {
                unsigned int line_width = 0;
                int radius = 3;
                point position;
            public:
                circle(json json_data) : shape(json_data, shape_type::CIRCLE) { this->read(json_data); }
                void read(json json_data) override;
                json::object_t get_json() override;
            };
                            
            class label : public shape {
                enum class alignment {
                    LEFT, RIGHT, CENTER
                };
                
                enum class baseline_types {
                    ALPHABETIC,
                    MIDDLE,
                    HANGING
                };
            
                alignment align = alignment::LEFT;
                baseline_types baseline = baseline_types::ALPHABETIC;
                std::string font_family;
                std::string text;
                int font_size = 10;
                point position;
            public:
                label(json json_data) : shape(json_data, shape_type::LABEL) { this->read(json_data); }
                std::string get_text() { return this->text; }
                void read(json json_data) override;
                json::object_t get_json() override;
            };
            
            class line : public shape {
                unsigned int width = 0;
                point start, end;
            protected:
                line(json json_data, shape_type type) : shape(json_data, type) { this->read(json_data); }
            public:
                line(json json_data) : line(json_data, shape_type::LINE) { }
                virtual void read(json json_data) override;
                virtual json::object_t get_json() override;
            };
            
            class rounded_segment : public line {
                int radius = 3;
            public:
                rounded_segment(json json_data) : line(json_data, shape_type::ROUNDED_SEGMENT) { this->read(json_data); }
                void read(json json_data) override;
                json::object_t get_json() override;
            };
            
            class polygon : public shape {
                unsigned int line_width = 0;
                std::vector<point> points;
                std::vector<shape_type> shape_types;
            public:
                polygon(json json_data) : shape(json_data, shape_type::POLYGON) { this->read(json_data); }
                void read(json json_data) override;
                json::object_t get_json() override;
            };
            
            class bezier_curve : public shape {
                point start, end, control_point1, control_point2;
            public:
                bezier_curve(json json_data) : shape(json_data, shape_type::BEZIER_CURVE) { this->read(json_data); }
                void read(json json_data) override;
                json::object_t get_json() override;
            };
            
            // Factory
            template<class shape_class, typename = std::enable_if<std::is_base_of<shape, shape_class>::value>>
            std::shared_ptr<shape> create(json json_data) {
                return std::shared_ptr<shape>(new shape_class(json_data));
            }
        };
        
        class annotation : public json_object {
            float rotation = 0.0f;
            point position;
            bool flip = false, visible = true;
            std::shared_ptr<shapes::label> label;
        public:
            annotation(json json_data) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        class symbol_attribute : public json_object {
            float rotation = 0.0f;
            point position;
            bool flip = false, hidden = false;
            std::vector<std::shared_ptr<annotation>> annotations;
        public:
            symbol_attribute(json json_data) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        class footprint_attribute : public json_object {
            float rotation = 0.0f;
            point position;
            bool flip = false;
            std::string layer_name;
        public:
            footprint_attribute(json json_data) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        class action_region : public json_object {
            std::map<std::string, std::string> attributes;
            std::map<std::string, std::string> styles;
            std::vector<std::vector<int>> connections;
            std::string name;
            point p1, p2;
            std::string ref_id;
        public:
            action_region(json json_data) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        class body : public json_object {
            float rotation = 0.0f;
            std::vector<int> connections;
            bool flip = false, moveable = true, removeable = true;
            std::string layer_name;
            std::vector<std::shared_ptr<shapes::shape>> shapes;
            std::vector<std::shared_ptr<action_region>> action_regions;
            std::vector<std::shared_ptr<annotation>> annotations;
            std::map<std::string, std::string> styles;
        public:
            body(json json_data) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        // Why are there two different generated object objects????
        class generated_object_attribute : public json_object {
            std::map<std::string, std::string> attributes;
            std::string layer_name;
            bool flip = false;
            float rotation = 0.0f;
            point position;
        public:
            generated_object_attribute(json json_data) { this->read(json_data); }
            virtual void read(json json_data) override;
            virtual json::object_t get_json() override;
        };
            
        class generated_object : public generated_object_attribute {
            std::vector<int> connections;
        public:
            generated_object(json json_data) : generated_object_attribute(json_data) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };

        class symbol : public json_object {
            std::vector<std::shared_ptr<body>> bodies;
        public:
            symbol(json json_data) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        class footprint : public json_object {
            std::vector<std::shared_ptr<body>> bodies;
            std::vector<std::shared_ptr<generated_object>> generated_objects;
        public:
            footprint(json json_data) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        class component : public json_object {
            std::string library_id, name;
            std::map<std::string, std::string> attributes;
            std::vector<footprint> footprints;
            std::vector<symbol> symbols;
        public:
            component(json json_data, std::string id) : library_id(id) { this->read(json_data); }
            std::string get_library_id() { return this->library_id; }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        class component_instance : public json_object {
            typedef struct {
                bool flip = false;
                float rotation = 0.0f;
                std::string side;
                point position;
            } footprint_pos_data;
            
            std::shared_ptr<component> component_def;
            std::map<std::string, std::string> attributes;
            std::vector<symbol_attribute> symbol_attributes;
            std::vector<footprint_attribute> footprint_attributes;
            std::vector<generated_object_attribute> generated_object_attributes;
            std::string instance_id;
            footprint_pos_data footprint_pos;
            int symbol_index = 0, footprint_index = 0;
        public:
            component_instance(std::shared_ptr<component> def, json json_data) : component_def(def) { read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
            std::string get_id() { return instance_id; }
        };
        
        class net_point : public json_object {
           typedef struct connected_action_region {
                unsigned int action_region_index;
                unsigned int body_index;
                std::string component_instance_id;
                int order_index = 0;
                std::string signal_name;
                connected_action_region(unsigned int action_region, unsigned int body, std::string component_instance, int order, std::string signal) :
                    action_region_index(action_region), body_index(body), component_instance_id(component_instance), order_index(order), signal_name(signal) {}
            } connected_action_region;
            
            std::string point_id;
            std::vector<connected_action_region> connected_action_regions;
            std::vector<std::string> connected_point_ids;
            point position;
        public:
            net_point(json json_data, std::string id) : point_id(id) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        class net : public json_object {
            // Are there actually any other net types?
            enum class type {
                NETS
            };
            
            std::vector<std::shared_ptr<annotation>> annotations;
            std::map<std::string, std::string> attributes;
            std::string net_id;
            type net_type = type::NETS;
            std::map<std::string, std::shared_ptr<net_point>> points;
            std::vector<std::string> signals;
        public:
            net(json json_data) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
            std::string get_id() { return net_id; }
        };
        
        class trace : public json_object {
            enum class type {
                STRAIGHT
            };
            
            std::string layer_name;
            point start, end;
            std::vector<point> control_points;
            type trace_type = type::STRAIGHT;
            double width;
        public:
            trace(json json_data) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        enum class pour_polygon_type {
            GENERAL_POLYGON_SET,
            GENERAL_POLYGON
        };
        
        class pour_polygon_base : public json_object {
            pour_polygon_type type;
            bool flip = false;
            float rotation = 0.0f;
            std::map<std::string, std::string> styles;
        public:
            pour_polygon_base(json json_data, pour_polygon_type poly_type) : type(poly_type) { this->read(json_data); }
            virtual void read(json json_data) override;
            virtual json::object_t get_json() override;
        };
        
        class pour_polygon : public pour_polygon_base {
            typedef struct {
                std::vector<point> points;
            } polygon_points;
            
            unsigned int line_width = 10;
            std::vector<polygon_points> holes;
            polygon_points pour_outline;
        public:
            pour_polygon(json json_data) : pour_polygon_base(json_data, pour_polygon_type::GENERAL_POLYGON) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        class pour_polygon_set : public pour_polygon_base {
            std::vector<std::shared_ptr<pour_polygon_base>> sub_polygons;
        public:
            pour_polygon_set(json json_data) : pour_polygon_base(json_data, pour_polygon_type::GENERAL_POLYGON_SET) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        class pour : public json_object {
            std::string attached_net_id;
            std::map<std::string, std::string> attributes;
            std::string layer_name;
            int order_index = 0;
            std::vector<point> points;
            std::shared_ptr<pour_polygon_base> pour_polygons;
            std::vector<shapes::shape_type> shape_types;
        public:
            pour(json json_data) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        class pcb_text : public json_object {
            bool flip = false, visible = true;
            std::shared_ptr<shapes::label> label;
            std::string layer_name;
            float rotation = 0.0f;
            point position;
        public:
            pcb_text(json json_data) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        // Why are there two different layout objects????
        class layout_body_attribute : public json_object {
            bool flip = false;
            std::string layer_name;
            float rotation = 0.0f;
            point position;
        public:
            layout_body_attribute(json json_data) { this->read(json_data); }
            virtual void read(json json_data) override;
            virtual json::object_t get_json() override;
        };
        
        class layout_object : public layout_body_attribute {
            std::map<std::string, std::string> attributes;
            std::vector<int> connections;
        public:
            layout_object(json json_data) : layout_body_attribute(json_data) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };

        class layer_option : public json_object {
        public:
            std::string ident, name;
            bool is_copper = true;
        public:
            layer_option(json json_data) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        class design_info : public json_object {
        public:
            typedef struct {
                std::vector<std::string> attached_links;
                std::string description;
                std::string design_id;
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
            design_info(json json_data) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
    };
    
    class data : public types::json_object {
    public:
        typedef struct {
            std::string exporter;
            int major = 0, minor = 2, build = 0;
        } version;
        
        version version_info;
        std::shared_ptr<types::design_info> design_info;
        
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
        data(json json_data) { this->read(json_data); }
        void read(json json_data) override;
        json::object_t get_json() override;
    };
    
    class open_json_format : public eda_format {
    private:
        std::vector<std::shared_ptr<data>> parsed_data;
    public:
        void read(std::initializer_list<std::string> files) override;
        void write(output_type type, std::string out_file) override;
    };
};