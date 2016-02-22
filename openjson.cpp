#include <chrono>
#include <istream>
#include <fstream>

#include "converter.hpp"
#include "openjson.hpp"

// Factory
open_json::types::shapes::shape_registry_type open_json::types::shapes::shape_registry = {
    {shape_type::LABEL, &create<label>},
    {shape_type::LINE, &create<line>}
};
std::map<std::string, open_json::types::shapes::shape_type> open_json::types::shapes::shape_typename_registry = {
    {"rectangle", shape_type::RECTANGLE},
    {"rounded_rectangle", shape_type::ROUNDED_RECTANGLE},
    {"arc", shape_type::ARC},
    {"circle", shape_type::CIRCLE},
    {"label", shape_type::LABEL},
    {"line", shape_type::LINE},
    {"polygon", shape_type::POLYGON},
    {"bezier", shape_type::BEZIER_CURVE}
};

//Data
void open_json::data::read(json json_data) {
    if (json_data.find("version") != json_data.end()) {
        std::vector<std::string> tokens = split(json_data["version"].value("file_version", "0.2.0"), "\\.");
        if (tokens.size() >= 3) {
            try {
                this->version_info.major = std::stoi(tokens[0]);
                this->version_info.minor = std::stoi(tokens[1]);
                this->version_info.build = std::stoi(tokens[2]);
            } catch(...) {
                std::cerr<<"Invalid file version assuming: "<<this->version_info.major<<"."<<this->version_info.minor<<"."<<this->version_info.build<<std::endl;
            }
        }
        this->version_info.exporter = json_data["version"].value("exporter", "None");
    }
    
    if (json_data.find("design_attributes") != json_data.end()) {
        design_info = std::shared_ptr<types::design_info>(new types::design_info(json_data["design_info"]));
    }
    
    if (json_data.find("components") != json_data.end()) {
        json componentsJson = json_data["components"];
        for (json::iterator it = componentsJson.begin(); it != componentsJson.end(); it++) {
            this->components[it.key()] = std::shared_ptr<types::component>(new types::component(it.value(), it.key()));
        }
    }
    
    if (json_data.find("component_instances") != json_data.end()) {
        json::array_t component_instances = json_data["component_instances"];
        for (json::object_t json_object : component_instances) {
            if (json_object.find("library_id") == json_object.end()) {
                throw new parse_exception("Component instance has no component library id!");
            }
            if (this->components[json_object["library_id"]] == nullptr) {
                throw new parse_exception("Component instance has an invalid component library id!\nDid you forget to add a library?");
            }
            std::shared_ptr<types::component_instance> componentInstance(new types::component_instance(this->components[json_object["library_id"]], json_object));
            this->component_instances[componentInstance->get_id()] = componentInstance;
        }
    }
    
    if (json_data.find("layer_options") != json_data.end()) {
        for (json::object_t json_object : json_data["layer_options"]) {
            this->layer_options.emplace_back(new types::layer_option(json_object));
        }
    }
    
    if (json_data.find("layout_bodies") != json_data.end()) {
        for (json::object_t json_object : json_data["layout_bodies"]) {
            this->layout_bodies.emplace_back(new types::body(json_object));
        }
    }
    
    if (json_data.find("layout_body_attributes") != json_data.end()) {
        for (json::object_t json_object : json_data["layout_body_attributes"]) {
            this->layout_body_attributes.emplace_back(new types::layout_body_attribute(json_object));
        }
    }
    
    if (json_data.find("layout_objects") != json_data.end()) {
        json::array_t layout_objects = json_data["layout_objects"];
        for (json::object_t json_object : layout_objects) {
            this->layout_objects.emplace_back(new types::layout_object(json_object));
        }
    }
    
    if (json_data.find("nets") != json_data.end()) {
        for (json::object_t json_object : json_data["nets"]) {
            if (json_object.find("net_id") == json_object.end()) {
                throw new parse_exception("Net has no net id!");
            }
            std::shared_ptr<types::net> net(new types::net(json_object));
            this->nets[net->get_id()] = net;
        }
    }
    
    if (json_data.find("pcb_text") != json_data.end()) {
        for (json::object_t json_object : json_data["pcb_text"]) {
            this->pcb_text.emplace_back(new types::pcb_text(json_object));
        }
    }
    
    if (json_data.find("pours") != json_data.end()) {
        for (json::object_t json_object : json_data["pours"]) {
            this->pours.emplace_back(new types::pour(json_object));
        }
    }
    
    if (json_data.find("trace_segments") != json_data.end()) {
        for (json::object_t json_object : json_data["trace_segments"]) {
            this->traces.emplace_back(new types::trace(json_object));
        }
    }
}

void open_json::data::write(json json_data) {
    
}

// Design Info

void open_json::types::design_info::read(json json_data) {
    if (json_data.find("annotations") != json_data.end()) {
        for (json::object_t json_object : json_data["annotations"]) {
            this->annotations.emplace_back(new types::annotation(json_object));
        }
    }
    
    if (json_data.find("attributes") != json_data.end()) {
        types::populate_attributes(this->attributes, json_data["attributes"]);
    }

    
    if (json_data.find("metadata") != json_data.end()) {
        for (std::string url : json_data["metadata"]["attached_urls"]) {
            this->metadata.attached_links.push_back(url);
        }
        
        this->metadata.description = json_data["metadata"].value("description", "");
        this->metadata.design_id = json_data["metadata"].value("design_id", "0000000000000000");
        this->metadata.license = json_data["metadata"].value("license", "Unknown");
        this->metadata.name = json_data["metadata"].value("name", "Untitled");
        this->metadata.owner = json_data["metadata"].value("owner", "Unknown");
        this->metadata.slug = json_data["metadata"].value("slug", this->metadata.name);
        this->metadata.last_updated = json_data["metadata"].value("updated_timestamp", std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    }
}

void open_json::types::design_info::write(json json_data) {
}

// Component

void open_json::types::component::read(json json_data) {
    if (json_data.find("attributes") != json_data.end()) {
        types::populate_attributes(this->attributes, json_data["attributes"]);
    }
    
    if (json_data.find("footprints") != json_data.end()) {
        for (json::object_t footprint_object : json_data["footprints"]) {
            this->footprints.emplace_back(footprint_object);
        }
    }
    
    if (json_data.find("symbols") != json_data.end()) {
        for (json symbol_object : json_data["symbols"]) {
            this->symbols.emplace_back(symbol_object);
        }
    }
}

void open_json::types::component::write(json json_data) {
    
}

// Component Instance

void open_json::types::component_instance::read(json json_data) {
    this->instance_id = json_data.value("instance_id", "0000000000000000");
    this->symbol_index = json_data.value("symbol_index", this->symbol_index);
    this->footprint_index = json_data.value("footprint_index", this->footprint_index);
    
    if (json_data.find("attributes") != json_data.end()) {
        types::populate_attributes(this->attributes, json_data["attributes"]);
    }

    if (json_data.find("gen_obj_attributes") != json_data.end()) {
        for (json::object_t json_object : json_data["gen_obj_attributes"]) {
            this->generated_object_attributes.emplace_back(json_object);
        }
    }
    
    if (json_data.find("symbol_attributes") != json_data.end()) {
        for (json::object_t json_object : json_data["symbol_attributes"]) {
            this->symbol_attributes.emplace_back(json_object);
        }
    }
    
    if (json_data.find("footprint_attributes") != json_data.end()) {
        for (json::object_t json_object : json_data["footprint_attributes"]) {
            this->footprint_attributes.emplace_back(json_object);
        }
    }
    
    if (json_data.find("footprint_pos") != json_data.end()) {
        json json_object = json_data["footprint_pos"];
        this->footprint_pos.flip = open_json::get_boolean(json_object["flip"]);
        this->footprint_pos.layer_name = json_object.value("layer", "Unnamed");
        this->footprint_pos.rotation = json_object.value("rotation", this->footprint_pos.rotation);
        this->footprint_pos.position = open_json::types::point(json_object.value("x", this->footprint_pos.position.x), json_object.value("y", this->footprint_pos.position.y));
    }
}

void open_json::types::component_instance::write(json json_data) {
    
}

// Footprint

void open_json::types::footprint::read(json json_data) {
    if (json_data.find("bodies") != json_data.end()) {
        for (json::object_t json_object : json_data["bodies"]) {
            this->bodies.emplace_back(new types::body(json_object));
        }
    }
    
    if (json_data.find("gen_objs") != json_data.end()) {
        for (json::object_t json_object : json_data["gen_objs"]) {
            this->generated_objects.emplace_back(new types::generated_object(json_object));
        }
    }
}

void open_json::types::footprint::write(json json_data) {
    
}

// Footprint Attribute

void open_json::types::footprint_attribute::read(json json_data) {
    this->rotation = json_data.value("rotation", this->rotation);
    this->flip = open_json::get_boolean(json_data["flip"]);
    this->position = open_json::types::point(json_data.value("x", this->position.x), json_data.value("y", this->position.y));
    this->layer_name = json_data.value("layer", "Unnamed");
}

void open_json::types::footprint_attribute::write(json json_data) {
    
}


// Symbol

void open_json::types::symbol::read(json json_data) {
    if (json_data.find("bodies") != json_data.end()) {
        for (json::object_t json_object : json_data["bodies"]) {
            this->bodies.emplace_back(new types::body(json_object));
        }
    }
}

void open_json::types::symbol::write(json json_data) {
    
}

// Symbol Attribute

void open_json::types::symbol_attribute::read(json json_data) {
    this->rotation = json_data.value("rotation", this->rotation);
    this->position = open_json::types::point(json_data.value("x", this->position.x), json_data.value("y", this->position.y));
    this->flip = open_json::get_boolean(json_data["flip"]);
    this->hidden = open_json::get_boolean(json_data["hidden"]);
    
    if (json_data.find("annotations") != json_data.end()) {
        json::array_t annotations = json_data["annotations"];
        for (json::object_t json_object : annotations) {
            this->annotations.emplace_back(new types::annotation(json_object));
        }
    }
}

void open_json::types::symbol_attribute::write(json json_data) {
    
}

// Body

void open_json::types::body::read(json json_data) {
    this->rotation = json_data.value("rotation", this->rotation);
    this->flip = open_json::get_boolean(json_data["flip"]);
    this->moveable = open_json::get_boolean(json_data["moveable"], true);
    this->removeable = open_json::get_boolean(json_data["removeable"], true);
    this->layer_name = json_data.value("layer", "Unnamed");
    
    if (json_data.find("connections") != json_data.end()) {
        for (int connection : json_data["connections"]) {
            this->connections.push_back(connection);
        }
    }
    
    if (json_data.find("styles") != json_data.end()) {
        open_json::types::populate_attributes(this->styles, json_data["styles"]);
    }
    
    if (json_data.find("shapes") != json_data.end()) {
        for (json::object_t json_object : json_data["shapes"]) {
            if (json_object.find("type") == json_object.end()) {
                throw new parse_exception("Invalid shape! Shape has no type specifier!");
            }
            if (open_json::types::shapes::shape_typename_registry.find(json_object["type"]) == open_json::types::shapes::shape_typename_registry.end()) {
                throw new parse_exception("Invalid shape type specified: " + json_object["type"].get<std::string>() + "!");
            }
            this->shapes.push_back(open_json::types::shapes::shape::new_shape(open_json::types::shapes::shape_typename_registry[json_object["type"]], json_object));
        }
    }
    
    if (json_data.find("action_regions") != json_data.end()) {
        for (json::object_t json_object : json_data["action_regions"]) {
            this->action_regions.emplace_back(new types::action_region(json_object));
        }
    }
    
    if (json_data.find("annotations") != json_data.end()) {
        for (json::object_t json_object : json_data["annotations"]) {
            this->annotations.emplace_back(new types::annotation(json_object));
        }
    }
}

void open_json::types::body::write(json json_data) {
    
}

// Generated Object

void open_json::types::generated_object::read(json json_data) {
    if (json_data.find("connections") != json_data.end()) {
        for (int connection : json_data["connections"]) {
            this->connections.push_back(connection);
        }
    }
}

void open_json::types::generated_object::write(json json_data) {
    
}

// Generated Object Attribute

void open_json::types::generated_object_attribute::read(json json_data) {
    this->layer_name = json_data.value("layer", "Unnamed");
    this->flip = open_json::get_boolean(json_data["flip"]);
    this->rotation = json_data.value("rotation", this->rotation);
    this->position = open_json::types::point(json_data.value("x", this->position.x), json_data.value("y", this->position.y));
    
    if (json_data.find("attributes") != json_data.end()) {
        types::populate_attributes(this->attributes, json_data["attributes"]);
    }
}

void open_json::types::generated_object_attribute::write(json json_data) {
    
}
// Action Region

void open_json::types::action_region::read(json json_data) {
    this->name = json_data.value("name", "Unnamed Region");
    this->ref_id = json_data.value("ref", "0000000000000000");
    
    if (json_data.find("attributes") != json_data.end()) {
        types::populate_attributes(this->attributes, json_data["attributes"]);
    }
    
    if (json_data.find("styles") != json_data.end()) {
        open_json::types::populate_attributes(this->styles, json_data["styles"]);
    }
    
    if (json_data.find("connections") != json_data.end()) {
        for (std::vector<int> connection : json_data["connections"]) {
            this->connections.push_back(connection);
        }
    }
    
    if (json_data.find("p1") != json_data.end()) {
        this->p1 = open_json::types::point(json_data["p1"].value("x", this->p1.x), json_data["p1"].value("y", this->p1.y));
    }
    
    if (json_data.find("p2") != json_data.end()) {
        this->p2 = open_json::types::point(json_data["p2"].value("x", this->p2.x), json_data["p2"].value("y", this->p2.y));
    }
}

void open_json::types::action_region::write(json json_data) {
    
}

// Annotation

void open_json::types::annotation::read(json json_data) {
    this->rotation = json_data.value("rotation", this->rotation);
    this->position = open_json::types::point(json_data.value("x", this->position.x), json_data.value("y", this->position.y));
    this->flip = open_json::get_boolean(json_data["flip"]);
    this->visible = open_json::get_boolean(json_data["visible"], true);
    
    if (json_data.find("label") != json_data.end()) {
        this->label = std::shared_ptr<open_json::types::shapes::label>(new open_json::types::shapes::label(json_data["label"]));
    }
}

void open_json::types::annotation::write(json json_data) {
    
}

// Layer Option

void open_json::types::layer_option::read(json json_data) {
    this->ident = json_data.value("ident", json_data.value("name", "Unnamed"));
    this->name = json_data.value("name", this->ident);
    this->is_copper = open_json::get_boolean(json_data["is_copper"], true);
}

void open_json::types::layer_option::write(json json_data) {
    
}

// Layout Object Attribute

void open_json::types::layout_body_attribute::read(json json_data) {
    this->flip = open_json::get_boolean(json_data["flip"]);
    this->layer_name = json_data.value("layer", "Unnamed");
    this->rotation = json_data.value("rotation", this->rotation);
    this->position = open_json::types::point(json_data.value("x", this->position.x), json_data.value("y", this->position.y));
}

void open_json::types::layout_body_attribute::write(json json_data) {
    
}

// Layout Object

void open_json::types::layout_object::read(json json_data) {
    if (json_data.find("attributes") != json_data.end()) {
        types::populate_attributes(this->attributes, json_data["attributes"]);
    }
    
    if (json_data.find("connections") != json_data.end()) {
        for (int connection : json_data["connections"]) {
            this->connections.push_back(connection);
        }
    }
}

void open_json::types::layout_object::write(json json_data) {
    
}

// PCB Text

void open_json::types::pcb_text::read(json json_data) {
    this->flip = open_json::get_boolean(json_data["flip"]);
    this->visible = open_json::get_boolean(json_data["visible"], true);
    this->layer_name = json_data.value("layer", "Unnamed");
    this->rotation = json_data.value("rotation", this->rotation);
    this->position = open_json::types::point(json_data.value("x", this->position.x), json_data.value("y", this->position.y));
    
    if (json_data.find("label") != json_data.end()) {
        this->label = std::shared_ptr<open_json::types::shapes::label>(new open_json::types::shapes::label(json_data["label"]));
    }
}

void open_json::types::pcb_text::write(json json_data) {
    
}

// Pour

void open_json::types::pour::read(json json_data) {
    this->attached_net_id = json_data.value("attached_net", "Unnamed");
    this->layer_name = json_data.value("layer", "Unnamed");
    this->order_index = json_data.value("rotation", this->order_index);
    
    if (json_data.find("attributes") != json_data.end()) {
        types::populate_attributes(this->attributes, json_data["attributes"]);
    }
    
    for (json::object_t point : json_data["points"]) {
        if (point.find("x") != point.end() && point.find("y") != point.end()) {
            this->points.emplace_back(point["x"], point["y"]);
        }
    }
    
    for (std::string shape_type_name : json_data["shape_types"]) {
        if (open_json::types::shapes::shape_typename_registry.find(shape_type_name) == open_json::types::shapes::shape_typename_registry.end()) {
            throw new parse_exception("Invalid shape type specified: " + shape_type_name + "!");
        }
        this->shape_types.push_back(open_json::types::shapes::shape_typename_registry[shape_type_name]);
    }
}

void open_json::types::pour::write(json json_data) {
    
}

// Trace

void open_json::types::trace::read(json json_data) {
    this->layer_name = json_data.value("layer", "Unnamed");
    this->width = json_data.value("width", 254000.0);

    if (json_data.find("p1") != json_data.end()) {
        this->start = open_json::types::point(json_data["p1"].value("x", this->start.x), json_data["p1"].value("y", this->start.y));
    }
    
    if (json_data.find("p2") != json_data.end()) {
        this->end = open_json::types::point(json_data["p2"].value("x", this->end.x), json_data["p2"].value("y", this->end.y));
    }
    
    for (json::object_t point : json_data["control_points"]) {
        if (point.find("x") != point.end() && point.find("y") != point.end()) {
            this->control_points.emplace_back(point["x"], point["y"]);
        }
    }
    
    if (json_data.find("trace_type") != json_data.end()) {
        // TODO This seems a bit pointless right now, I have only ever seen straight
        if (json_data["trace_type"] == "straight") {
            this->trace_type = trace::type::STRAIGHT;
        }
    }
}

void open_json::types::trace::write(json json_data) {
    
}

// Net

void open_json::types::net::read(json json_data) {
    this->net_id = json_data.value("net_id", "0000000000000000");
    
    if (json_data.find("net_type") != json_data.end()) {
        if (json_data["net_type"] == "nets") {
            this->net_type = net::type::NETS;
        }
    }
    
    if (json_data.find("annotations") != json_data.end()) {
        json::array_t annotations = json_data["annotations"];
        for (json::object_t json_object : annotations) {
            this->annotations.emplace_back(new types::annotation(json_object));
        }
    }
    
    if (json_data.find("attributes") != json_data.end()) {
        types::populate_attributes(this->attributes, json_data["attributes"]);
    }
    
    if (json_data.find("points") != json_data.end()) {
        for (json::object_t net_object : json_data["points"]) {
            if (net_object.find("point_id") == net_object.end()) {
                throw new parse_exception("Invalid point in net: " + this->net_id + "! Point does not contain a point id!");
            }
            this->points[net_object["point_id"]] = std::shared_ptr<net_point>(new net_point(json_data, net_object["point_id"]));
        }
    }
}

void open_json::types::net::write(json json_data) {
    
}

// Net Point

void open_json::types::net_point::read(json json_data) {
    this->position = open_json::types::point(json_data.value("x", this->position.x), json_data.value("y", this->position.y));
    
    if (json_data.find("connected_action_regions") != json_data.end()) {
        for (json connected_action_region : json_data["connected_action_regions"]) {
            this->connected_action_regions.emplace_back(
                connected_action_region.value("action_region_index", 0),
                connected_action_region.value("body_index", 0),
                connected_action_region.value("instance_id", "0000000000000000"),
                connected_action_region.value("order", 0),
                connected_action_region.value("signal", "")
            );
        }
    }
    
    for (std::string point_id : json_data["connected_points"]) {
        this->connected_point_ids.push_back(point_id);
    }
}

void open_json::types::net_point::write(json json_data) {
    
}

// Shapes
// Shape

void open_json::types::shapes::shape::read(json json_data) {
    this->flip = open_json::get_boolean(json_data["flip"]);
    this->rotation = json_data.value("rotation", this->rotation);
    
    if (json_data.find("styles") != json_data.end()) {
        open_json::types::populate_attributes(this->styles, json_data["styles"]);
    }
}

void open_json::types::shapes::shape::write(json json_data) {
    
}

// Label

void open_json::types::shapes::label::read(json json_data) {
    // Default to sans serif
    this->font_family = json_data.value("font_family", "sans-serif");
    this->font_size = json_data.value("font_size", this->font_size);
    this->position = open_json::types::point(json_data.value("x", this->position.x), json_data.value("y", this->position.y));
    this->text = json_data.value("text", "");
    
    if (json_data.find("align") != json_data.end()) {
        if (json_data["align"] == "left") {
            this->align = alignment::LEFT;
        } else if (json_data["align"] == "right") {
            this->align = alignment::RIGHT;
        } else if (json_data["align"] == "center") {
            this->align = alignment::CENTER;
        }
    }
    
    if (json_data.find("baseline") != json_data.end()) {
        if (json_data["baseline"] == "alphabetic") {
            this->baseline = baseline_types::ALPHABET;
        }
    }
}

void open_json::types::shapes::label::write(json json_data) {
    
}

// Line

void open_json::types::shapes::line::read(json json_data) {
    this->width = json_data.value("width", this->width);
    
    if (json_data.find("p1") != json_data.end()) {
        this->start = open_json::types::point(json_data["p1"].value("x", this->start.x), json_data["p1"].value("y", this->start.y));
    }
    
    if (json_data.find("p2") != json_data.end()) {
        this->end = open_json::types::point(json_data["p2"].value("x", this->end.x), json_data["p2"].value("y", this->end.y));
    }
}

void open_json::types::shapes::line::write(json json_data) {
    
}

// OpenJSON

void open_json::open_json_format::read(std::initializer_list<std::string> files) {
    std::vector<std::shared_ptr<data>> parsedData;
    for (std::string file : files) {
        std::ifstream file_stream(file);
        json raw_json_data;
        file_stream >> raw_json_data;
        parsedData.emplace_back(new data(raw_json_data));
        file_stream.close();
    }
}

void open_json::open_json_format::write(output_type type, std::string out_file) {
    
}