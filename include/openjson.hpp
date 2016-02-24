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
    // Forward declare data
    class data;
    
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
    
    // Unfouturnatly there seems to be a good number of null values where there shouldn't be in OpenJSON outputed by Upverter. Custom getter to prevent exceptions.
    template<typename value_type>
    inline value_type get_value_or_default(json object, std::string key, value_type default_value) {
        if (object.is_null()) {
            return default_value;
        }
        value_type return_value;
        try {
            return_value = object.value(key, default_value);
        } catch (...) {
            return_value = default_value;
        }
        return return_value;
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
        protected:
            json_object *parent;
        public:
            json_object(json_object *super) : parent(super) {}
            virtual void read(json json_data) = 0;
            virtual json::object_t get_json() = 0;
        };
        
        namespace shapes {
            enum class shape_type {
                RECTANGLE, ROUNDED_RECTANGLE, ARC, CIRCLE, LABEL, LINE, ROUNDED_SEGMENT, POLYGON, BEZIER_CURVE
            };
            
            class shape;
            typedef std::shared_ptr<shape> (*create_function)(json_object *super, open_json::data *file, json json_data);
            typedef std::map<shape_type, create_function> shape_registry_type;
            extern shape_registry_type shape_registry;
            extern std::map<std::string, shape_type> shape_typename_registry;
            extern std::map<shape_type, std::string> name_shape_type_registry;
            
            class shape : public json_object {
            private:
                open_json::data *file_data;
            public:
                shape_type type;
                std::map<std::string, std::string> styles;
                float rotation = 0.0f;
                bool flip = false;
            public:
                shape (json_object *super, open_json::data *file, json json_data, shape_type shape_type) : json_object(super), file_data(file), type(shape_type) { this->read(json_data); }
                virtual void read(json json_data) override;
                virtual json::object_t get_json() override;
                static std::shared_ptr<shape> new_shape(shape_type type, json_object *super, open_json::data *file, json json_data) {
                    if (shape_registry.find(type) == shape_registry.end()) {
                        return nullptr;
                    }
                    return shape_registry[type](super, file, json_data);
                }
            };
            
            class rectangle : public shape {
                unsigned int line_width = 0;
                int width = 0, height = 0;
                point position;
            protected:
                rectangle(json_object *super, open_json::data *file, json json_data, shape_type type) : shape(super, file, json_data, type) { this->read(json_data); }
            public:
                rectangle(json_object *super, open_json::data *file, json json_data) : rectangle(super, file, json_data, shape_type::RECTANGLE) {}
                virtual void read(json json_data) override;
                virtual json::object_t get_json() override;
            };
            
            class rounded_rectangle : public rectangle {
                int radius = 3; // Corner rounding radius
            public:
                rounded_rectangle(json_object *super, open_json::data *file, json json_data) : rectangle(super, file, json_data, shape_type::ROUNDED_RECTANGLE) { this->read(json_data); }
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
                arc(json_object *super, open_json::data *file, json json_data) : shape(super, file, json_data, shape_type::ARC) { this->read(json_data); }
                void read(json json_data) override;
                json::object_t get_json() override;
            };
            
            class circle : public shape {
                unsigned int line_width = 0;
                int radius = 3;
                point position;
            public:
                circle(json_object *super, open_json::data *file, json json_data) : shape(super, file, json_data, shape_type::CIRCLE) { this->read(json_data); }
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
                label(json_object *super, open_json::data *file, json json_data) : shape(super, file, json_data, shape_type::LABEL) { this->read(json_data); }
                std::string get_text() { return this->text; }
                void read(json json_data) override;
                json::object_t get_json() override;
            };
            
            class line : public shape {
                unsigned int width = 0;
                point start, end;
            protected:
                line(json_object *super, open_json::data *file, json json_data, shape_type type) : shape(super, file, json_data, type) { this->read(json_data); }
            public:
                line(json_object *super, open_json::data *file, json json_data) : line(super, file, json_data, shape_type::LINE) { }
                virtual void read(json json_data) override;
                virtual json::object_t get_json() override;
            };
            
            class rounded_segment : public line {
                int radius = 3;
            public:
                rounded_segment(json_object *super, open_json::data *file, json json_data) : line(super, file, json_data, shape_type::ROUNDED_SEGMENT) { this->read(json_data); }
                void read(json json_data) override;
                json::object_t get_json() override;
            };
            
            class polygon : public shape {
                unsigned int line_width = 0;
                std::vector<point> points;
                std::vector<shape_type> shape_types;
            public:
                polygon(json_object *super, open_json::data *file, json json_data) : shape(super, file, json_data, shape_type::POLYGON) { this->read(json_data); }
                void read(json json_data) override;
                json::object_t get_json() override;
            };
            
            class bezier_curve : public shape {
                point start, end, control_point1, control_point2;
            public:
                bezier_curve(json_object *super, open_json::data *file, json json_data) : shape(super, file, json_data, shape_type::BEZIER_CURVE) { this->read(json_data); }
                void read(json json_data) override;
                json::object_t get_json() override;
            };
            
            // Factory
            template<class shape_class, typename = std::enable_if<std::is_base_of<shape, shape_class>::value>>
            std::shared_ptr<shape> create(json_object *super, open_json::data *file, json json_data) {
                return std::shared_ptr<shape>(new shape_class(super, file, json_data));
            }
        };
        
        class annotation : public json_object {
        private:
            open_json::data *file_data;
            float rotation = 0.0f;
            point position;
            bool flip = false, visible = true;
            std::shared_ptr<shapes::label> label;
        public:
            annotation(json_object *super, open_json::data *file, json json_data) : json_object(super), file_data(file) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        class symbol_attribute : public json_object {
        private:
            open_json::data *file_data;
            float rotation = 0.0f;
            point position;
            bool flip = false, hidden = false;
            std::vector<std::shared_ptr<annotation>> annotations;
        public:
            symbol_attribute(json_object *super, open_json::data *file, json json_data) : json_object(super), file_data(file) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        class footprint_attribute : public json_object {
        private:
            open_json::data *file_data;
            float rotation = 0.0f;
            point position;
            bool flip = false;
            std::string layer_name;
        public:
            footprint_attribute(json_object *super, open_json::data *file, json json_data) : json_object(super), file_data(file){ this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        class action_region : public json_object {
        private:
            open_json::data *file_data;
            std::map<std::string, std::string> attributes;
            std::map<std::string, std::string> styles;
            std::vector<std::vector<int>> connections;
            std::string name;
            point p1, p2;
            std::string ref_id;
        public:
            action_region(json_object *super, open_json::data *file, json json_data) : json_object(super), file_data(file) { this->read(json_data); }
            std::string get_ref_id() { return this->ref_id; }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        class body : public json_object {
        private:
            open_json::data *file_data;
            float rotation = 0.0f;
            std::vector<int> connections;
            bool flip = false, moveable = true, removeable = true;
            std::string layer_name;
            std::vector<std::shared_ptr<shapes::shape>> shapes;
            std::vector<std::shared_ptr<action_region>> action_regions;
            std::vector<std::shared_ptr<annotation>> annotations;
            std::map<std::string, std::string> styles;
        public:
            body(json_object *super, open_json::data *file, json json_data) : json_object(super), file_data(file) { this->read(json_data); }
            void add_shape(std::shared_ptr<shapes::shape> shape) { this->shapes.push_back(shape); }
            size_t get_number_of_action_regions() { return this->action_regions.size(); }
            std::shared_ptr<types::action_region> get_action_region_at_index(size_t index) { return index < this->action_regions.size() ? this->action_regions[index] : std::shared_ptr<types::action_region>(); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        // Why are there two different generated object objects????
        class generated_object_attribute : public json_object {
        protected:
            open_json::data *file_data;
            std::map<std::string, std::string> attributes;
            std::string layer_name;
            bool flip = false;
            float rotation = 0.0f;
            point position;
        public:
            generated_object_attribute(json_object *super, open_json::data *file, json json_data) : json_object(super), file_data(file) { this->read(json_data); }
            virtual void read(json json_data) override;
            virtual json::object_t get_json() override;
        };
        
        class generated_object : public generated_object_attribute {
            std::vector<int> connections;
        public:
            generated_object(json_object *super, open_json::data *file, json json_data) : generated_object_attribute(super, file, json_data) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };

        class symbol : public json_object {
        private:
            open_json::data *file_data;
            std::vector<std::shared_ptr<body>> bodies;
        public:
            symbol(json_object *super, open_json::data *file, json json_data) : json_object(super), file_data(file) { this->read(json_data); }
            size_t get_number_of_bodies() { return this->bodies.size(); }
            std::shared_ptr<types::body> get_body_at_index(size_t index) { return index < this->bodies.size() ? this->bodies[index] : std::shared_ptr<types::body>(); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        class footprint : public json_object {
        private:
            open_json::data *file_data;
            std::vector<std::shared_ptr<body>> bodies;
            std::vector<std::shared_ptr<generated_object>> generated_objects;
        public:
            footprint(json_object *super, open_json::data *file, json json_data) : json_object(super), file_data(file) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        class component : public json_object {
        private:
            open_json::data *file_data;
            std::string library_id, name;
            std::map<std::string, std::string> attributes;
            std::vector<footprint> footprints;
            std::vector<std::shared_ptr<symbol>> symbols;
        public:
            component(json_object *super, open_json::data *file, json json_data, std::string id) : json_object(super), file_data(file), library_id(id){ this->read(json_data); }
            std::string get_library_id() { return this->library_id; }
            size_t get_number_of_symbols() { return this->symbols.size(); }
            std::shared_ptr<types::symbol> get_symbol_at_index(size_t index) { return index < this->symbols.size() ? this->symbols[index] : std::shared_ptr<types::symbol>(); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        class component_instance : public json_object {
            typedef struct {
                bool flip = false;
                float rotation = 0.0f;
                std::string side = "top";
                point position;
            } footprint_pos_data;
        private:
            open_json::data *file_data;
            std::shared_ptr<component> component_def;
            std::map<std::string, std::string> attributes;
            std::vector<symbol_attribute> symbol_attributes;
            std::vector<footprint_attribute> footprint_attributes;
            std::vector<generated_object_attribute> generated_object_attributes;
            std::string instance_id;
            footprint_pos_data footprint_pos;
            size_t symbol_index = 0, footprint_index = 0;
        public:
            component_instance(json_object *super, open_json::data *file, std::shared_ptr<component> def, json json_data) : json_object(super), file_data(file), component_def(def) { read(json_data); }
            std::string get_id() { return instance_id; }
            size_t get_symbol_index() { return this->symbol_index; }
            std::shared_ptr<types::component> get_definition() { return this->component_def; }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        class net_point : public json_object {
           typedef struct connected_action_region {
                size_t action_region_index;
                size_t body_index;
                std::string component_instance_id;
                int order_index = 0;
                std::string signal_name;
                connected_action_region(size_t action_region, size_t body, std::string component_instance, int order, std::string signal) :
                    action_region_index(action_region), body_index(body), component_instance_id(component_instance), order_index(order), signal_name(signal) {}
            } connected_action_region;
        private:
            open_json::data *file_data;
            std::string point_id;
            std::vector<connected_action_region> connected_action_regions;
            std::vector<std::string> connected_point_ids;
            point position;
        public:
            net_point(json_object *super, open_json::data *file, std::string id) : json_object(super), file_data(file), point_id(id) { }
            std::vector<connected_action_region>::iterator get_begining_of_connected_regions() { return this->connected_action_regions.begin(); }
            std::vector<connected_action_region>::iterator get_end_of_connected_regions() { return this->connected_action_regions.end(); }
            std::vector<std::string> get_connected_point_ids() { return this->connected_point_ids; }
            bool try_read(json json_data);  
            virtual void read(json json_data) override { try_read(json_data); }
            json::object_t get_json() override;
        };
        
        class net : public json_object {
            enum class type {
                NETS,
                MODULES_NETS
            };
        private:
            open_json::data *file_data;
            std::vector<std::shared_ptr<annotation>> annotations;
            std::map<std::string, std::string> attributes;
            std::string net_id;
            type net_type = type::NETS;
            std::map<std::string, std::shared_ptr<net_point>> points;
            std::vector<std::string> signals;
        public:
            net(json_object *super, open_json::data *file) : json_object(super), file_data(file) { }
            std::string get_id() { return net_id; }
            bool try_read(json json_data);  
            virtual void read(json json_data) override { try_read(json_data); }
            json::object_t get_json() override;
        };
        
        class trace : public json_object {
            enum class type {
                STRAIGHT
            };
        private:
            open_json::data *file_data;
            std::string layer_name;
            point start, end;
            std::vector<point> control_points;
            type trace_type = type::STRAIGHT;
            double width;
        public:
            trace(json_object *super, open_json::data *file, json json_data) : json_object(super), file_data(file) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        enum class pour_polygon_type {
            GENERAL_POLYGON_SET,
            GENERAL_POLYGON
        };
        
        class pour_polygon_base : public json_object {
        protected:
            open_json::data *file_data;
            pour_polygon_type type;
            bool flip = false;
            float rotation = 0.0f;
            std::map<std::string, std::string> styles;
        public:
            pour_polygon_base(json_object *super, open_json::data *file, json json_data, pour_polygon_type poly_type) : json_object(super), file_data(file), type(poly_type) { this->read(json_data); }
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
            pour_polygon(json_object *super, open_json::data *file, json json_data) : pour_polygon_base(super, file, json_data, pour_polygon_type::GENERAL_POLYGON) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        class pour_polygon_set : public pour_polygon_base {
            std::vector<std::shared_ptr<pour_polygon_base>> sub_polygons;
        public:
            pour_polygon_set(json_object *super, open_json::data *file, json json_data) : pour_polygon_base(super, file, json_data, pour_polygon_type::GENERAL_POLYGON_SET) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        class pour : public json_object {
        private:
            open_json::data *file_data;
            std::string attached_net_id;
            std::map<std::string, std::string> attributes;
            std::string layer_name;
            int order_index = 0;
            std::vector<point> points;
            std::shared_ptr<pour_polygon_base> pour_polygons;
            std::vector<shapes::shape_type> shape_types;
        public:
            pour(json_object *super, open_json::data *file, json json_data) : json_object(super), file_data(file) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        class pcb_text : public json_object {
        private:
            open_json::data *file_data;
            bool flip = false, visible = true;
            std::shared_ptr<shapes::label> label;
            std::string layer_name;
            float rotation = 0.0f;
            point position;
        public:
            pcb_text(json_object *super, open_json::data *file, json json_data) : json_object(super), file_data(file) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        // Why are there two different layout objects????
        class layout_body_attribute : public json_object {
        protected:
            open_json::data *file_data;
            bool flip = false;
            std::string layer_name;
            float rotation = 0.0f;
            point position;
        public:
            layout_body_attribute(json_object *super, open_json::data *file, json json_data) : json_object(super), file_data(file) { this->read(json_data); }
            virtual void read(json json_data) override;
            virtual json::object_t get_json() override;
        };
        
        class layout_object : public layout_body_attribute {
            std::map<std::string, std::string> attributes;
            std::vector<int> connections;
        public:
            layout_object(json_object *super, open_json::data *file, json json_data) : layout_body_attribute(super, file, json_data) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };

        class layer_option : public json_object {
        private:
            open_json::data *file_data;
            std::string ident, name;
            bool is_copper = true;
        public:
            layer_option(json_object *super, open_json::data *file, json json_data) : json_object(super), file_data(file) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        class path : public json_object {
        private:
            open_json::data *file_data;
            std::map<std::string, std::string> attributes;
            bool is_closed = true;
            std::string layer_name;
            double width = 250000.0; // .25mm
            std::vector<point> points;
            std::vector<shapes::shape_type> shape_types;
        public:
            path(json_object *super, open_json::data *file, json json_data) : json_object(super), file_data(file) { this->read(json_data); }
            void read(json json_data) override;
            json::object_t get_json() override;
        };
        
        class design_info : public json_object {
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
        private:
            open_json::data *file_data;
            std::vector<std::shared_ptr<annotation>> annotations;
            std::map<std::string, std::string> attributes;
            metadata_container metadata;
        public:
            design_info(json_object *super, open_json::data *file, json json_data) : json_object(super), file_data(file) { this->read(json_data); }
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
        std::string original_file_name;
        
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
        std::vector<std::shared_ptr<types::path>> paths;
    public:
        data(std::string file_name, json json_data) : json_object(nullptr), original_file_name(file_name) { this->read(json_data); }
        void read(json json_data) override;
        json::object_t get_json() override;
    };
    
    class open_json_format : public eda_format {
    private:
        std::vector<std::shared_ptr<data>> parsed_data;
    public:
        void read(std::vector<std::string> files) override;
        void write(output_type type, std::string out_file) override;
    };
};