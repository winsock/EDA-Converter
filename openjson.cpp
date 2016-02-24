#include <chrono>
#include <istream>
#include <ostream>
#include <fstream>

#include "converter.hpp"
#include "openjson.hpp"

// Factory
open_json::types::shapes::shape_registry_type open_json::types::shapes::shape_registry = {
    {shape_type::RECTANGLE, &create<rectangle>},
    {shape_type::ROUNDED_RECTANGLE, &create<rounded_rectangle>},
    {shape_type::ARC, &create<arc>},
    {shape_type::CIRCLE, &create<circle>},
    {shape_type::LABEL, &create<label>},
    {shape_type::LINE, &create<line>},
    {shape_type::ROUNDED_SEGMENT, &create<rounded_segment>},
    {shape_type::POLYGON, &create<polygon>},
    {shape_type::BEZIER_CURVE, &create<bezier_curve>}
};

std::map<std::string, open_json::types::shapes::shape_type> open_json::types::shapes::shape_typename_registry = {
    {"rectangle", shape_type::RECTANGLE},
    {"rounded_rectangle", shape_type::ROUNDED_RECTANGLE},
    {"arc", shape_type::ARC},
    {"circle", shape_type::CIRCLE},
    {"label", shape_type::LABEL},
    {"line", shape_type::LINE},
    {"rounded_segment", shape_type::ROUNDED_SEGMENT},
    {"polygon", shape_type::POLYGON},
    {"bezier", shape_type::BEZIER_CURVE}
};

std::map<open_json::types::shapes::shape_type, std::string> open_json::types::shapes::name_shape_type_registry = {
    {shape_type::RECTANGLE, "rectangle"},
    {shape_type::ROUNDED_RECTANGLE, "rounded_rectangle"},
    {shape_type::ARC, "arc"},
    {shape_type::CIRCLE, "circle"},
    {shape_type::LABEL, "label"},
    {shape_type::LINE, "line"},
    {shape_type::ROUNDED_SEGMENT, "rounded_segment"},
    {shape_type::POLYGON, "polygon"},
    {shape_type::BEZIER_CURVE, "bezier"}
};

// Data
void open_json::data::read(json json_data) {
    if (json_data.find("version") != json_data.end()) {
        // TODO Move the hard-coded current version number
        std::vector<std::string> tokens = split(open_json::get_value_or_default<std::string>(json_data["version"], "file_version", "0.2.0"), "\\.");
        if (tokens.size() >= 3) {
            try {
                this->version_info.major = std::stoi(tokens[0]);
                this->version_info.minor = std::stoi(tokens[1]);
                this->version_info.build = std::stoi(tokens[2]);
            } catch(...) {
                std::cerr<<"Invalid file version assuming: "<<this->version_info.major<<"."<<this->version_info.minor<<"."<<this->version_info.build<<std::endl;
            }
        }
        this->version_info.exporter = open_json::get_value_or_default<std::string>(json_data["version"], "exporter", "None");
    }
    
    if (this->version_info.major < 1 && this->version_info.minor < 2) {
        // TODO Move the hard-coded current version number
        std::cout<<"Attempting to upgrade the file format from version: "<<this->version_info.major<<"."<<this->version_info.minor<<"."<<this->version_info.build<<" to 0.2.0"<<std::endl;
    }
    
    if (json_data.find("design_attributes") != json_data.end()) {
        design_info = std::shared_ptr<types::design_info>(new types::design_info(dynamic_cast<types::json_object*>(this), this, json_data["design_info"]));
    }
    
    if (json_data.find("components") != json_data.end()) {
        json componentsJson = json_data["components"];
        for (json::iterator it = componentsJson.begin(); it != componentsJson.end(); it++) {
            this->components[it.key()] = std::shared_ptr<types::component>(new types::component(dynamic_cast<types::json_object*>(this), this, it.value(), it.key()));
        }
    }
    
    if (json_data.find("component_instances") != json_data.end()) {
        json::array_t component_instances = json_data["component_instances"];
        for (json::object_t json_object : component_instances) {
            if (json_object.find("library_id") == json_object.end()) {
                throw parse_exception("Component instance has no component library id!");
            }
            if (this->components[json_object["library_id"]] == nullptr) {
                std::cerr<<"Component instance does not have a matching component definition, not adding!"<<std::endl;
            }
            try {
                std::shared_ptr<types::component_instance> componentInstance(new types::component_instance(dynamic_cast<types::json_object*>(this), this, this->components[json_object["library_id"]], json_object));
                this->component_instances[componentInstance->get_id()] = componentInstance;
            } catch (parse_exception e) {
                std::cerr<<"Not adding component instance for reason:"<<e.what()<<std::endl;
            }
        }
    }
    
    if (json_data.find("layer_options") != json_data.end()) {
        for (json::object_t json_object : json_data["layer_options"]) {
            this->layer_options.emplace_back(new types::layer_option(dynamic_cast<types::json_object*>(this), this, json_object));
        }
    }
    
    if (json_data.find("layout_bodies") != json_data.end()) {
        for (json::object_t json_object : json_data["layout_bodies"]) {
            this->layout_bodies.emplace_back(new types::body(dynamic_cast<types::json_object*>(this), this, json_object));
        }
    }
    
    if (json_data.find("layout_body_attributes") != json_data.end()) {
        for (json::object_t json_object : json_data["layout_body_attributes"]) {
            this->layout_body_attributes.emplace_back(new types::layout_body_attribute(dynamic_cast<types::json_object*>(this), this, json_object));
        }
    }
    
    if (json_data.find("layout_objects") != json_data.end()) {
        json::array_t layout_objects = json_data["layout_objects"];
        for (json::object_t json_object : layout_objects) {
            this->layout_objects.emplace_back(new types::layout_object(dynamic_cast<types::json_object*>(this), this, json_object));
        }
    }
    
    if (json_data.find("nets") != json_data.end()) {
        for (json::object_t json_object : json_data["nets"]) {
            if (json_object.find("net_id") == json_object.end()) {
                throw parse_exception("Net has no net id!");
            }
            std::shared_ptr<types::net> net(new types::net(dynamic_cast<types::json_object*>(this), this));
            try {
                if (net->try_read(json_object)) {
                    this->nets[net->get_id()] = net;
                    continue;
                }
            } catch (parse_exception e) {
                std::cerr<<"Invalid net found! Skipping net:"<<net->get_id()<<" Reason: "<<e.what()<<std::endl;
                continue;
            }
            std::cerr<<"Invalid net found! Skipping net:"<<net->get_id()<<std::endl;
        }
    }
    
    if (json_data.find("pcb_text") != json_data.end()) {
        for (json::object_t json_object : json_data["pcb_text"]) {
            this->pcb_text.emplace_back(new types::pcb_text(dynamic_cast<types::json_object*>(this), this, json_object));
        }
    }
    
    if (json_data.find("pours") != json_data.end()) {
        for (json::object_t json_object : json_data["pours"]) {
            this->pours.emplace_back(new types::pour(dynamic_cast<types::json_object*>(this), this, json_object));
        }
    }
    
    if (json_data.find("trace_segments") != json_data.end()) {
        for (json::object_t json_object : json_data["trace_segments"]) {
            this->traces.emplace_back(new types::trace(dynamic_cast<types::json_object*>(this), this, json_object));
        }
    }
}

json::object_t open_json::data::get_json() {
    json data = {
        {"component_instances", json::value_t::array},
        {"components", json::value_t::object},
        {"design_attributes", json::value_t::object},
        {"layer_options", json::value_t::array},
        {"layout_bodies", json::value_t::array},
        {"layout_body_attributes", json::value_t::array},
        {"layout_objects", json::value_t::array},
        {"nets", json::value_t::array},
        {"pcb_text", json::value_t::array},
        {"pours", json::value_t::array},
        {"trace_segments", json::value_t::array},
        {"version", { // TODO Move this constant to a const/possibly a cli option to change name
            {"exporter", "EDA Converter"}, 
            {"file_version", "0.2.0"} // We export the 0.2.0 version, TODO move this to a const
        }}
    };
    
    for (auto component_instance : this->component_instances) {
        data["component_instances"].push_back(component_instance.second->get_json());
        // Only write out the component definitions we need!
        data["components"][component_instance.second->get_definition()->get_library_id()] = component_instance.second->get_definition()->get_json();
    }
    
    // for (auto component : this->components) {
    //     data["components"][component.first] = component.second->get_json();
    // }
    
    if (this->design_info.get() != nullptr) {
        data["design_attributes"] = this->design_info->get_json();
    }
    
    for (auto layer_option : this->layer_options) {
        data["layer_option"].push_back(layer_option->get_json());
    }
    
    for (auto layout_body : this->layout_bodies) {
        data["layout_bodies"].push_back(layout_body->get_json());
    }
    
    for (auto layout_body_attribute : this->layout_body_attributes) {
        data["layout_body_attributes"].push_back(layout_body_attribute->get_json());
    }
    
    for (auto layout_object : this->layout_objects) {
        data["layout_objects"].push_back(layout_object->get_json());
    }
    
    for (auto net : this->nets) {
        data["nets"].push_back(net.second->get_json());
    }
    
    for (auto pcb_text_object : this->pcb_text) {
        data["pcb_text"].push_back(pcb_text_object->get_json());
    }
    
    for (auto pour : this->pours) {
        data["pours"].push_back(pour->get_json());
    }
    
    for (auto trace : this->traces) {
        data["trace_segments"].push_back(trace->get_json());
    }
    
    return data;
}

// Design Info
void open_json::types::design_info::read(json json_data) {
    if (json_data.find("annotations") != json_data.end()) {
        for (json::object_t json_object : json_data["annotations"]) {
            this->annotations.emplace_back(new types::annotation(dynamic_cast<types::json_object*>(this), this->file_data, json_object));
        }
    }
    
    if (json_data.find("attributes") != json_data.end()) {
        types::populate_attributes(this->attributes, json_data["attributes"]);
    }

    
    if (json_data.find("metadata") != json_data.end()) {
        for (std::string url : json_data["metadata"]["attached_urls"]) {
            this->metadata.attached_links.push_back(url);
        }
        
        this->metadata.description = open_json::get_value_or_default<std::string>(json_data["metadata"], "description", "");
        this->metadata.design_id = open_json::get_value_or_default<std::string>(json_data["metadata"], "design_id", "0000000000000000");
        this->metadata.license = open_json::get_value_or_default<std::string>(json_data["metadata"], "license", "Unknown");
        this->metadata.name = open_json::get_value_or_default<std::string>(json_data["metadata"], "name", "Untitled");
        this->metadata.owner = open_json::get_value_or_default<std::string>(json_data["metadata"], "owner", "Unknown");
        this->metadata.slug = open_json::get_value_or_default(json_data["metadata"], "slug", this->metadata.name);
        this->metadata.last_updated = open_json::get_value_or_default(json_data["metadata"], "updated_timestamp", std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    }
}

json::object_t open_json::types::design_info::get_json() {
    json data {
        {"annotations", json::value_t::array},
        {"attributes", this->attributes},
        {"metadata", {
            {"attached_urls", this->metadata.attached_links},
            {"description", this->metadata.description},
            {"design_id", this->metadata.design_id},
            {"license", this->metadata.license},
            {"name", this->metadata.name},
            {"owner", this->metadata.owner},
            {"slug", this->metadata.slug},
            {"updated_timestamp", this->metadata.last_updated}
        }}
    };
    for (auto a : this->annotations) {
        data["annotations"].push_back(a->get_json());
    }
    return data;
}

// Component
void open_json::types::component::read(json json_data) {
    this->name = open_json::get_value_or_default<std::string>(json_data, "name", "Unamed");
    
    if (json_data.find("attributes") != json_data.end()) {
        types::populate_attributes(this->attributes, json_data["attributes"]);
    }
    
    if (json_data.find("footprints") != json_data.end()) {
        for (json::object_t footprint_object : json_data["footprints"]) {
            this->footprints.emplace_back(dynamic_cast<types::json_object*>(this), this->file_data, footprint_object);
        }
    }
    
    if (json_data.find("symbols") != json_data.end()) {
        for (json symbol_object : json_data["symbols"]) {
            this->symbols.emplace_back(new types::symbol(dynamic_cast<types::json_object*>(this), this->file_data, symbol_object));
        }
    }
}

json::object_t open_json::types::component::get_json() {
    json data {
        {"attributes", this->attributes},
        {"footprints", json::value_t::array},
        {"name", this->name},
        {"symbols", json::value_t::array}
    };
    for (auto f : this->footprints) {
        data["footprints"].push_back(f.get_json());
    }
    for (auto s : this->symbols) {
        data["symbols"].push_back(s->get_json());
    }
    return data;
}

// Component Instance
void open_json::types::component_instance::read(json json_data) {
    this->instance_id = open_json::get_value_or_default<std::string>(json_data, "instance_id", "0000000000000000");
    this->symbol_index = open_json::get_value_or_default(json_data, "symbol_index", this->symbol_index);
    this->footprint_index = open_json::get_value_or_default(json_data, "footprint_index", this->footprint_index);
    
    if (json_data.find("attributes") != json_data.end()) {
        types::populate_attributes(this->attributes, json_data["attributes"]);
    }

    if (json_data.find("gen_obj_attributes") != json_data.end()) {
        for (json::object_t json_object : json_data["gen_obj_attributes"]) {
            this->generated_object_attributes.emplace_back(dynamic_cast<types::json_object*>(this), this->file_data, json_object);
        }
    }
    
    if (json_data.find("symbol_attributes") != json_data.end()) {
        for (json::object_t json_object : json_data["symbol_attributes"]) {
            this->symbol_attributes.emplace_back(dynamic_cast<types::json_object*>(this), this->file_data, json_object);
        }
    }
    
    if (json_data.find("footprint_attributes") != json_data.end()) {
        for (json::object_t json_object : json_data["footprint_attributes"]) {
            this->footprint_attributes.emplace_back(dynamic_cast<types::json_object*>(this), this->file_data, json_object);
        }
    }
    
    if (json_data.find("footprint_pos") != json_data.end()) {
        json json_object = json_data["footprint_pos"];
        this->footprint_pos.flip = open_json::get_boolean(json_object["flip"]);
        this->footprint_pos.side = open_json::get_value_or_default(json_object, "side", this->footprint_pos.side);
        this->footprint_pos.rotation = open_json::get_value_or_default(json_object, "rotation", this->footprint_pos.rotation);
        this->footprint_pos.position = open_json::types::point(open_json::get_value_or_default(json_object, "x", this->footprint_pos.position.x), open_json::get_value_or_default(json_object, "y", this->footprint_pos.position.y));
    }
}

json::object_t open_json::types::component_instance::get_json() {
    json data = {
        {"attributes", this->attributes},
        {"footprint_attributes", json::value_t::array},
        {"footprint_index", this->footprint_index},
        {"footprint_pos", {
            {"flip", this->footprint_pos.flip},
            {"rotation", this->footprint_pos.rotation},
            {"side", this->footprint_pos.side},
            {"x", this->footprint_pos.position.x},
            {"y", this->footprint_pos.position.y}}},
        {"gen_obj_attributes", json::value_t::array},
        {"instance_id", this->instance_id},
        {"library_id", this->component_def->get_library_id()},
        {"symbol_attributes", json::value_t::array},
        {"symbol_index", this->symbol_index}
    };
    
    for (auto f : this->footprint_attributes) {
        data["footprint_attributes"].push_back(f.get_json());
    }
    
    for (auto o : this->generated_object_attributes) {
        data["gen_obj_attributes"].push_back(o.get_json());
    }
    
    for (auto s : this->symbol_attributes) {
        data["symbol_attributes"].push_back(s.get_json());
    }
    
    return data;
}

// Footprint
void open_json::types::footprint::read(json json_data) {
    if (json_data.find("bodies") != json_data.end()) {
        for (json::object_t json_object : json_data["bodies"]) {
            this->bodies.emplace_back(new types::body(dynamic_cast<types::json_object*>(this), this->file_data, json_object));
        }
    }
    
    if (json_data.find("gen_objs") != json_data.end()) {
        for (json::object_t json_object : json_data["gen_objs"]) {
            this->generated_objects.emplace_back(new types::generated_object(dynamic_cast<types::json_object*>(this), this->file_data, json_object));
        }
    }
}

json::object_t open_json::types::footprint::get_json() {
    json data = {
        {"bodies", json::value_t::array},
        {"gen_objs", json::value_t::array}
    };
    for (auto b : this->bodies) {
        data["bodies"].push_back(b->get_json());
    }
    for (auto o : this->generated_objects) {
        data["gen_objs"].push_back(o->get_json());
    }
    return data;
}

// Footprint Attribute
void open_json::types::footprint_attribute::read(json json_data) {
    this->rotation = open_json::get_value_or_default(json_data, "rotation", this->rotation);
    this->flip = open_json::get_boolean(json_data["flip"]);
    this->position = open_json::types::point(open_json::get_value_or_default(json_data, "x", this->position.x), open_json::get_value_or_default(json_data, "y", this->position.y));
    try {
        if (json_data["layer"].is_null()) {
            throw parse_exception("Layer name is null! Most likely this is from a ghost component instance!");
        }
        this->layer_name = open_json::get_value_or_default<std::string>(json_data, "layer", "Unnamed");
    } catch (std::exception e) {
        throw parse_exception(std::string("Error parsing layer name! Exception: ").append(e.what()));
    }
}

json::object_t open_json::types::footprint_attribute::get_json() {
    return {
        {"flip", this->flip},
        {"rotation", this->rotation},
        {"layer", this->layer_name},
        {"x", this->position.x},
        {"y", this->position.y}
    };
}


// Symbol
void open_json::types::symbol::read(json json_data) {
    if (json_data.find("bodies") != json_data.end()) {
        for (json::object_t json_object : json_data["bodies"]) {
            this->bodies.emplace_back(new types::body(dynamic_cast<types::json_object*>(this), this->file_data, json_object));
        }
    }
}

json::object_t open_json::types::symbol::get_json() {
    json data = {
        {"bodies", json::value_t::array}
    };
    for (auto b : this->bodies) {
        data["bodies"].push_back(b->get_json());
    }
    return data;
}

// Symbol Attribute
void open_json::types::symbol_attribute::read(json json_data) {
    this->rotation = open_json::get_value_or_default(json_data, "rotation", this->rotation);
    this->position = open_json::types::point(open_json::get_value_or_default(json_data, "x", this->position.x), open_json::get_value_or_default(json_data, "y", this->position.y));
    this->flip = open_json::get_boolean(json_data["flip"]);
    this->hidden = open_json::get_boolean(json_data["hidden"]);
    
    if (json_data.find("annotations") != json_data.end()) {
        json::array_t annotations = json_data["annotations"];
        for (json::object_t json_object : annotations) {
            this->annotations.emplace_back(new types::annotation(dynamic_cast<types::json_object*>(this), this->file_data, json_object));
        }
    }
}

json::object_t open_json::types::symbol_attribute::get_json() {
    json data = {
        {"annotations", json::value_t::array},
        {"flip", this->flip},
        {"hidden", this->hidden},
        {"rotation", this->rotation},
        {"x", this->position.x},
        {"y", this->position.y}
    };
    for (auto a : this->annotations) {
        data["annotations"].push_back(a->get_json());
    }
    return data;
}

// Body
void open_json::types::body::read(json json_data) {
    this->rotation = open_json::get_value_or_default(json_data, "rotation", this->rotation);
    this->flip = open_json::get_boolean(json_data["flip"]);
    this->moveable = open_json::get_boolean(json_data["moveable"], true);
    this->removeable = open_json::get_boolean(json_data["removeable"], true);
    this->layer_name = open_json::get_value_or_default<std::string>(json_data, "layer", "Unnamed");
    
    if (json_data.find("connection_indexes") != json_data.end()) {
        for (int connection : json_data["connection_indexes"]) {
            this->connections.push_back(connection);
        }
    }
    
    if (json_data.find("styles") != json_data.end()) {
        open_json::types::populate_attributes(this->styles, json_data["styles"]);
    }
    
    if (json_data.find("shapes") != json_data.end()) {
        for (json::object_t json_object : json_data["shapes"]) {
            if (json_object.find("type") == json_object.end()) {
                throw parse_exception("Invalid shape! Shape has no type specifier!");
            }
            if (open_json::types::shapes::shape_typename_registry.find(json_object["type"]) == open_json::types::shapes::shape_typename_registry.end()) {
                throw parse_exception("Invalid shape type specified: " + json_object["type"].get<std::string>() + "!");
            }
            this->shapes.push_back(open_json::types::shapes::shape::new_shape(open_json::types::shapes::shape_typename_registry[json_object["type"]], dynamic_cast<types::json_object*>(this), this->file_data, json_object));
        }
    }
    
    if (this->file_data->version_info.major < 1 && this->file_data->version_info.minor < 2) {
        // Convert pins to action_regions
        if (json_data.find("pins") != json_data.end()) {
            for (json::object_t json_object : json_data["pins"]) {
                this->action_regions.emplace_back(new types::action_region(dynamic_cast<types::json_object*>(this), this->file_data, json_object));
            }
        }
    } else {
        if (json_data.find("action_regions") != json_data.end()) {
            for (json::object_t json_object : json_data["action_regions"]) {
                this->action_regions.emplace_back(new types::action_region(dynamic_cast<types::json_object*>(this), this->file_data, json_object));
            }
        }
    }
    
    if (json_data.find("annotations") != json_data.end()) {
        for (json::object_t json_object : json_data["annotations"]) {
            this->annotations.emplace_back(new types::annotation(dynamic_cast<types::json_object*>(this), this->file_data, json_object));
        }
    }
}

json::object_t open_json::types::body::get_json() {
    json data = {
        {"annotations", json::value_t::array},
        {"styles", this->styles},
        {"connection_indexes", json::value_t::array},
        {"shapes", json::value_t::array},
        {"action_regions", json::value_t::array},
        {"rotation", this->rotation},
        {"flip", this->flip},
        {"moveable", this->moveable},
        {"removeable", this->removeable},
        {"layer", this->layer_name}
    };
    
    for (auto a : this->annotations) {
        data["annotations"].push_back(a->get_json());
    }
    
    if (this->connections.size() > 0) {
        data["connection_indexes"] = this->connections;
    }
    
    for (auto s : this->shapes) {
        data["shapes"].push_back(s->get_json());
    }
    
    for (auto a : this->action_regions) {
        data["action_regions"].push_back(a->get_json());
    }
    
    return data;
}

// Generated Object
void open_json::types::generated_object::read(json json_data) {
    if (json_data.find("connection_indexes") != json_data.end()) {
        for (int connection : json_data["connection_indexes"]) {
            this->connections.push_back(connection);
        }
    }
}

json::object_t open_json::types::generated_object::get_json() {
    json data = generated_object_attribute::get_json();
    if (this->connections.size() > 0) {
        data["connection_indexes"] = this->connections;
    } else {
        data["connection_indexes"] = json::value_t::array;
    }
    return data;
}

// Generated Object Attribute
void open_json::types::generated_object_attribute::read(json json_data) {
    this->flip = open_json::get_boolean(json_data["flip"]);
    this->rotation = open_json::get_value_or_default(json_data, "rotation", this->rotation);
    this->position = open_json::types::point(open_json::get_value_or_default(json_data, "x", this->position.x), open_json::get_value_or_default(json_data, "y", this->position.y));
    
    try {
        if (json_data["layer"].is_null()) {
            throw parse_exception("Layer name is null! Most likely this is from a ghost component instance!");
        }
        this->layer_name = open_json::get_value_or_default<std::string>(json_data, "layer", "Unnamed");
    } catch (std::exception e) {
        throw parse_exception(std::string("Error parsing layer name! Exception: ").append(e.what()));
    }
    
    if (json_data.find("attributes") != json_data.end()) {
        types::populate_attributes(this->attributes, json_data["attributes"]);
    }
}

json::object_t open_json::types::generated_object_attribute::get_json() {
    return {
        {"attributes", this->attributes},
        {"flip", this->flip},
        {"layer", this->layer_name},
        {"rotation", this->rotation},
        {"x", this->position.x},
        {"y", this->position.y}
    };
}
// Action Region
void open_json::types::action_region::read(json json_data) {
    if (this->file_data->version_info.major < 1 && this->file_data->version_info.minor < 2) {
        // Pins to Action Region
        this->ref_id = this->name = open_json::get_value_or_default<std::string>(json_data, "pin_number", "0");
        // Move label to shapes
        if (json_data.find("label") != json_data.end()) {
            // Apparently you cannot count on the label object having the "type" field
            dynamic_cast<types::body*>(this->parent)->add_shape(open_json::types::shapes::shape::new_shape(open_json::types::shapes::shape_type::LABEL, this->parent, this->file_data, json_data["label"]));
            // Set the name to the label text, or if for some reason it doesn't exist use the pin_number text
            this->name = open_json::get_value_or_default(json_data["label"], "text", this->name);
        }
    } else {
        this->name = open_json::get_value_or_default<std::string>(json_data, "name", "Unnamed Region");
        this->ref_id = open_json::get_value_or_default(json_data, "ref", this->name);
    }

    if (json_data.find("attributes") != json_data.end()) {
        types::populate_attributes(this->attributes, json_data["attributes"]);
    }
    
    if (json_data.find("styles") != json_data.end()) {
        open_json::types::populate_attributes(this->styles, json_data["styles"]);
    }
    
    // No need to check for versions less than 0.2.0 since this will check for the existance of the "connections" key anyways
    if (json_data.find("connections") != json_data.end()) {
        for (std::vector<int> connection : json_data["connections"]) {
            this->connections.push_back(connection);
        }
    }
    
    if (json_data.find("p1") != json_data.end()) {
        this->p1 = open_json::types::point(open_json::get_value_or_default(json_data["p1"], "x", this->p1.x), open_json::get_value_or_default(json_data["p1"], "y", this->p1.y));
    }
    
    if (json_data.find("p2") != json_data.end()) {
        this->p2 = open_json::types::point(open_json::get_value_or_default(json_data["p2"], "x", this->p2.x), open_json::get_value_or_default(json_data["p2"], "y", this->p2.y));
    }
}

json::object_t open_json::types::action_region::get_json() {
    return {
        {"attributes", this->attributes},
        {"connections", this->connections},
        {"name", this->name},
        {"p1", {
            {"x", this->p1.x},
            {"y", this->p1.y}}},
        {"p2", {
            {"x", this->p2.x},
            {"y", this->p2.y}}},
        {"ref", this->ref_id},
        {"styles", this->styles}
    };
}

// Annotation
void open_json::types::annotation::read(json json_data) {
    this->rotation = open_json::get_value_or_default(json_data, "rotation", this->rotation);
    this->position = open_json::types::point(open_json::get_value_or_default(json_data, "x", this->position.x), open_json::get_value_or_default(json_data, "y", this->position.y));
    this->flip = open_json::get_boolean(json_data["flip"]);
    this->visible = open_json::get_boolean(json_data["visible"], true);
    
    if (json_data.find("label") != json_data.end()) {
        this->label = std::shared_ptr<open_json::types::shapes::label>(new open_json::types::shapes::label(dynamic_cast<types::json_object*>(this), this->file_data, json_data["label"]));
    }
}

json::object_t open_json::types::annotation::get_json() {
    json data = {
        {"flip", this->flip},
        {"rotation", this->rotation},
        {"value", (this->label.get() != nullptr) ? this->label->get_text() : ""},
        {"visible", this->visible ? "true" : "false"}, // WHYYYY sigh....
        {"x", this->position.x},
        {"y", this->position.y}
    };
    
    if (this->label.get() != nullptr) {
        data["label"] = this->label->get_json();
    }
    return data;
}

// Layer Option
void open_json::types::layer_option::read(json json_data) {
    this->ident = open_json::get_value_or_default(json_data, "ident", open_json::get_value_or_default<std::string>(json_data, "name", "Unnamed"));
    this->name = open_json::get_value_or_default(json_data, "name", this->ident);
    this->is_copper = open_json::get_boolean(json_data["is_copper"], true);
}

json::object_t open_json::types::layer_option::get_json() {
    return {
        {"ident", this->ident},
        {"is_copper", this->is_copper},
        {"name", this->name}
    };
}

// Layout Object Attribute
void open_json::types::layout_body_attribute::read(json json_data) {
    this->flip = open_json::get_boolean(json_data["flip"]);
    this->layer_name = open_json::get_value_or_default<std::string>(json_data, "layer", "Unnamed");
    this->rotation = open_json::get_value_or_default(json_data, "rotation", this->rotation);
    this->position = open_json::types::point(open_json::get_value_or_default(json_data, "x", this->position.x), open_json::get_value_or_default(json_data, "y", this->position.y));
}

json::object_t open_json::types::layout_body_attribute::get_json() {
    return {
        {"flip", this->flip},
        {"layer", this->layer_name},
        {"rotation", this->rotation},
        {"x", this->position.x},
        {"y", this->position.y}
    };
}

// Layout Object
void open_json::types::layout_object::read(json json_data) {
    if (json_data.find("attributes") != json_data.end()) {
        types::populate_attributes(this->attributes, json_data["attributes"]);
    }
    
    if (json_data.find("connection_indexes") != json_data.end() && !json_data["connection_indexes"].is_null()) {
        for (int connection : json_data["connection_indexes"]) {
            this->connections.push_back(connection);
        }
    }
}

json::object_t open_json::types::layout_object::get_json() {
    json data = layout_body_attribute::get_json();
    data["attributes"] = this->attributes;

    if (this->connections.size() > 0) {
        data["connection_indexes"] = this->connections;
    } else {
        data["connection_indexes"] = json::value_t::array;
    }
    return data;
}

// PCB Text
void open_json::types::pcb_text::read(json json_data) {
    this->flip = open_json::get_boolean(json_data["flip"]);
    this->visible = open_json::get_boolean(json_data["visible"], true);
    this->layer_name = open_json::get_value_or_default<std::string>(json_data, "layer", "Unnamed");
    this->rotation = open_json::get_value_or_default(json_data, "rotation", this->rotation);
    this->position = open_json::types::point(open_json::get_value_or_default(json_data, "x", this->position.x), open_json::get_value_or_default(json_data, "y", this->position.y));
    
    if (json_data.find("label") != json_data.end()) {
        this->label = std::shared_ptr<open_json::types::shapes::label>(new open_json::types::shapes::label(dynamic_cast<types::json_object*>(this), this->file_data, json_data["label"]));
    }
}

json::object_t open_json::types::pcb_text::get_json() {
    json data = {
        {"flip", this->flip},
        {"layer", this->layer_name},
        {"rotation", this->rotation},
        {"value", (this->label.get() != nullptr) ? this->label->get_text() : ""},
        {"visible", this->visible},
        {"x", this->position.x},
        {"y", this->position.y}
    };
    
    if (this->label.get() != nullptr) {
        data["label"] = this->label->get_json();
    }
    return data;
}

// Pour Polygon Base
void open_json::types::pour_polygon_base::read(json json_data) {
    this->flip = open_json::get_boolean(json_data["flip"]);
    this->rotation = open_json::get_value_or_default(json_data, "rotation", this->rotation);
    
    if (json_data.find("styles") != json_data.end()) {
        open_json::types::populate_attributes(this->styles, json_data["styles"]);
    }
}

json::object_t open_json::types::pour_polygon_base::get_json() {
    json data = {
        {"flip", this->flip},
        {"rotation", this->rotation},
        {"styles", this->styles}
    };
    
    switch (this->type) {
        case pour_polygon_type::GENERAL_POLYGON_SET:
            data["type"] = "general_polygon_set";
            break;
        case pour_polygon_type::GENERAL_POLYGON:
        default:
            data["type"] = "general_polygon";
            break;
    }
    return data;
}

// Pour Polygon
void open_json::types::pour_polygon::read(json json_data) {
    this->line_width = open_json::get_value_or_default(json_data, "line_width", this->line_width);
    
    if (json_data.find("holes") != json_data.end()) {
        for (json polygon : json_data["holes"]) {
            if (polygon.find("points") != polygon.end()) {
                std::vector<point> polygon_vertices;
                for (json::object_t point : polygon["points"]) {
                    if (point.find("x") != point.end() && point.find("y") != point.end()) {
                        polygon_vertices.push_back({point["x"], point["y"]});
                    }
                }
                this->holes.push_back({polygon_vertices});
            }
        }
    }
    
    if (json_data.find("outline") != json_data.end() && json_data["outline"].find("points") != json_data["outline"].end()) {
        for (json::object_t point : json_data["outline"]["points"]) {
            if (point.find("x") != point.end() && point.find("y") != point.end()) {
                this->pour_outline.points.push_back({point["x"], point["y"]});
            }
        }
    }
}

json::object_t open_json::types::pour_polygon::get_json() {
    json data = pour_polygon_base::get_json();
    data["line_width"] = this->line_width;
    data["holes"] = json::value_t::array; // Make empty array just in case there are no holes
    for (polygon_points hole : this->holes) {
        json hole_object = {{"points", json::value_t::array}};
        for (point p : hole.points) {
            hole_object["points"].push_back(json({
                {"x", p.x},
                {"y", p.y}
            }));
        }
        data["holes"].push_back(hole_object);
    }
    json outline = {{"points", json::value_t::array}};
    for (point p : this->pour_outline.points) {
        outline["points"].push_back(json({
            {"x", p.x},
            {"y", p.y}
        }));
    }
    data["outline"] = outline;
    return data;
}

// Pour Polygon Set
void open_json::types::pour_polygon_set::read(json json_data) {
    if (json_data.find("polygons") != json_data.end()) {
        for (json::object_t general_polygon : json_data["polygons"]) {
            if (general_polygon.find("type") == general_polygon.end()) {
                throw parse_exception("Invalid polygon in pour! No polygon type specified!");
            }
            
            if (general_polygon["type"] == "general_polygon") {
                this->sub_polygons.emplace_back(new types::pour_polygon(dynamic_cast<types::json_object*>(this), this->file_data, general_polygon));
            } else if (general_polygon["type"] == "general_polygon_set") {
                this->sub_polygons.emplace_back(new types::pour_polygon_set(dynamic_cast<types::json_object*>(this), this->file_data, general_polygon));
            }
        }
    }
}

json::object_t open_json::types::pour_polygon_set::get_json() {
    json data = pour_polygon_base::get_json();
    data["polygons"] = json::value_t::array; // Make empty array just in case there are no polygons
    for (auto sub_polygon : this->sub_polygons) {
        data["polygons"].push_back(sub_polygon->get_json());
    }
    return data;
}

// Pour
void open_json::types::pour::read(json json_data) {
    this->attached_net_id = open_json::get_value_or_default<std::string>(json_data, "attached_net", "Unnamed");
    this->layer_name = open_json::get_value_or_default<std::string>(json_data, "layer", "Unnamed");
    this->order_index = open_json::get_value_or_default(json_data, "rotation", this->order_index);
    
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
            throw parse_exception("Invalid shape type specified: " + shape_type_name + "!");
        }
        this->shape_types.push_back(open_json::types::shapes::shape_typename_registry[shape_type_name]);
    }
    
    if (json_data.find("polygons") != json_data.end()) {
        if (json_data["polygons"].find("type") == json_data["polygons"].end()) {
            throw parse_exception("Invalid polygon in pour! No polygon type specified!");
        }
        
        if (json_data["polygons"]["type"] == "general_polygon") {
            this->pour_polygons = std::shared_ptr<types::pour_polygon_base>(new types::pour_polygon(dynamic_cast<types::json_object*>(this), this->file_data, json_data["polygons"]));
        } else if (json_data["polygons"]["type"] == "general_polygon_set") {
            this->pour_polygons = std::shared_ptr<types::pour_polygon_base>(new types::pour_polygon_set(dynamic_cast<types::json_object*>(this), this->file_data, json_data["polygons"]));
        }
    }
}

json::object_t open_json::types::pour::get_json() {
    json data = {
        {"attached_net", this->attached_net_id},
        {"attributes", this->attributes},
        {"layer", this->layer_name},
        {"order", this->order_index},
        {"points", json::value_t::array},
        {"shape_types", json::value_t::array}
    };
    
    for (point &p : this->points) {
        data["points"].push_back(json({{"x", p.x}, {"y", p.y}}));
    }
    
    if (this->pour_polygons.get() != nullptr) {
        data["polygons"] = this->pour_polygons->get_json();
    }
    
    for (shapes::shape_type t : this->shape_types) {
        data["shape_types"].push_back(open_json::types::shapes::name_shape_type_registry[t]);
    }
    return data;
}

// Trace
void open_json::types::trace::read(json json_data) {
    this->layer_name = open_json::get_value_or_default<std::string>(json_data, "layer", "Unnamed");
    this->width = open_json::get_value_or_default(json_data, "width", 254000.0);

    if (json_data.find("p1") != json_data.end()) {
        this->start = open_json::types::point(open_json::get_value_or_default(json_data["p1"], "x", this->start.x), open_json::get_value_or_default(json_data["p1"], "y", this->start.y));
    }
    
    if (json_data.find("p2") != json_data.end()) {
        this->end = open_json::types::point(open_json::get_value_or_default(json_data["p2"], "x", this->end.x), open_json::get_value_or_default(json_data["p2"], "y", this->end.y));
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

json::object_t open_json::types::trace::get_json() {
    json data = {
        {"control_points", json::value_t::array},
        {"layer", this->layer_name},
        {"p1", {
            {"x", this->start.x},
            {"y", this->start.y}
        }},
        {"p2", {
            {"x", this->end.x},
            {"y", this->end.y}
        }},
        {"width", this->width}
    };
    
    for (point p : this->control_points) {
        data["control_points"].push_back(json({
            {"x", p.x},
            {"y", p.y}
        }));
    }
    
    switch (this->trace_type) {
        case type::STRAIGHT:
        default:
            data["trace_type"] = "straight";
            break;
    }
    
    return data;
}

// Net
bool open_json::types::net::try_read(json json_data) {
    this->net_id = open_json::get_value_or_default<std::string>(json_data, "net_id", "0000000000000000");
    
    if (json_data.find("net_type") != json_data.end()) {
        if (json_data["net_type"] == "nets") {
            this->net_type = net::type::NETS;
        } else if (json_data["net_type"] == "modules_nets") {
            this->net_type = net::type::MODULES_NETS;
        }
    }
    
    if (json_data.find("annotations") != json_data.end()) {
        json::array_t annotations = json_data["annotations"];
        for (json::object_t json_object : annotations) {
            this->annotations.emplace_back(new types::annotation(dynamic_cast<types::json_object*>(this), this->file_data, json_object));
        }
    }
    
    if (json_data.find("attributes") != json_data.end()) {
        types::populate_attributes(this->attributes, json_data["attributes"]);
    }
    
    if (json_data.find("points") != json_data.end()) {
        bool check_for_inconsistencies = false;
        for (json::object_t net_object : json_data["points"]) {
            if (net_object.find("point_id") == net_object.end()) {
                throw parse_exception("Invalid point in net: " + this->net_id + "! Point does not contain a point id!");
            }
            auto point = std::shared_ptr<net_point>(new net_point(dynamic_cast<types::json_object*>(this), this->file_data, net_object["point_id"]));
            if (!point->try_read(net_object)) {
                // Adding point failed for some reason check for any inconsistencies.
                check_for_inconsistencies = true;
            }
        }
        // Check data
        if (check_for_inconsistencies) {
            std::cout<<"Potentially inconsistent net: "<<this->net_id<<" checking consistency"<<std::endl;
            for (auto point : this->points) {
                for (auto action_region_iterator = point.second->get_begining_of_connected_regions(); action_region_iterator < point.second->get_end_of_connected_regions(); action_region_iterator++) {
                    if (this->file_data->component_instances.find(action_region_iterator->component_instance_id) == this->file_data->component_instances.end()) {
                        // Inconsistency found, tell caller to not add this net!
                        std::cerr<<"Inconsistency in action regions for net: "<<this->net_id<<" invalid component instance id, skipping net!"<<std::endl;
                        return false;
                    }
                }
                for (std::string id : point.second->get_connected_point_ids()) {
                    if (this->points.find(id) == this->points.end()) {
                        std::cerr<<"Inconsistency in connected points for net: "<<this->net_id<<" no such point with id: "<<id<<", skipping net!"<<std::endl;
                        return false;
                    }
                }
            }
            std::cout<<"The net: "<<this->net_id<<" seems consistent and repaired. Double check the net though maunually!"<<std::endl;
        }
    }
    
    if (json_data.find("signals") != json_data.end()) {
        for (std::string signal_name : json_data["signals"]) {
            this->signals.push_back(signal_name);
        }
    }
    
    return true;
}

json::object_t open_json::types::net::get_json() {
    json out = {
        {"annotations", json::value_t::array},
        {"attributes", this->attributes},
        {"net_id", this->net_id},
        {"points", json::value_t::array},
        {"signals", this->signals}
    };
    
    for (auto a : this->annotations) {
        out["annotations"].push_back(a->get_json());
    }
    
    switch (this->net_type) {
        case type::MODULES_NETS:
            out["net_type"] = "modules_nets";
            break;
        case type::NETS:
        default:
            out["net_type"] = "nets";
            break;
    }
    
    for (auto pointkv : this->points) {
        out["points"].push_back(pointkv.second->get_json());
    }
    
    return out;
}

// Net Point
bool open_json::types::net_point::try_read(json json_data) {
    this->position = open_json::types::point(open_json::get_value_or_default(json_data, "x", this->position.x), open_json::get_value_or_default(json_data, "y", this->position.y));
    
    for (std::string point_id : json_data["connected_points"]) {
        this->connected_point_ids.push_back(point_id);
    }
    
    // Wont be found in versions less than 0.2.0
    if (json_data.find("connected_action_regions") != json_data.end()) {
        for (json connected_action_region : json_data["connected_action_regions"]) {
            std::string component_instance = open_json::get_value_or_default<std::string>(connected_action_region, "instance_id", "0000000000000000");
            // Consistency check
            if (this->file_data->component_instances.find(component_instance) ==  this->file_data->component_instances.end()) {
                // Refers to invalid component instance!
                std::cerr<<"Error in connected action region for net_point:"<<this->point_id<<"! Invalid instance_id!"<<std::endl;
                return false;
            }
            this->connected_action_regions.emplace_back(
                open_json::get_value_or_default(connected_action_region, "action_region_index", 0),
                open_json::get_value_or_default(connected_action_region, "body_index", 0),
                component_instance,
                open_json::get_value_or_default(connected_action_region, "order", 0),
                open_json::get_value_or_default<std::string>(connected_action_region, "signal", "")
            );
        }
    }
    
    // Only in file versions less than 0.2.0
    if (json_data.find("connected_components") != json_data.end()) {
        for (json connected_component : json_data["connected_components"]) {
            if (connected_component.find("instance_id") == connected_component.end()) {
                std::cerr<<"Error in connected action region for net_point:"<<this->point_id<<"! A connected component does not have an instance id!"<<std::endl;
                return false;
            }
            if (connected_component.find("pin_number") == connected_component.end()) {
                std::cerr<<"Error in connected action region for net_point:"<<this->point_id<<"! A connected component does not have a pin number!"<<std::endl;
                return false;
            }
            if (this->file_data->component_instances.find(connected_component["instance_id"]) != this->file_data->component_instances.end()) {
                std::shared_ptr<types::component_instance> component_instance = this->file_data->component_instances[connected_component["instance_id"]];
                std::shared_ptr<types::symbol> symbol = component_instance->get_definition()->get_symbol_at_index(component_instance->get_symbol_index());
                if (!symbol) {
                    std::cerr<<"Error in connected action region for net_point:"<<this->point_id<<"! Invalid symbol index!"<<std::endl;
                    return false;
                }
                auto get_action_region_index = [&symbol, &connected_component](size_t body_index) -> std::pair<bool, size_t> {
                    for (size_t action_region_index = 0; action_region_index < symbol->get_body_at_index(body_index)->get_number_of_action_regions(); action_region_index++) {
                        std::shared_ptr<types::action_region> action_region = symbol->get_body_at_index(body_index)->get_action_region_at_index(action_region_index);
                        if (action_region->get_ref_id() == connected_component["pin_number"]) {
                            return std::make_pair(true, action_region_index);
                        }
                    }
                    return std::make_pair(false, 0);
                };
                bool conversion_successful = false;
                for (size_t body_index = 0; body_index < symbol->get_number_of_bodies(); body_index++) {
                    auto result = get_action_region_index(body_index);
                    // Go with the firt body with the matching ref id, the old format didn't store a body index value
                    if (result.first) {
                        this->connected_action_regions.emplace_back(
                            result.second,
                            body_index,
                            connected_component["instance_id"],
                            0,
                            ""
                        );
                        conversion_successful = true;
                        break;
                    }
                }
                if (!conversion_successful) {
                    std::cerr<<"Error converting a net point("<<this->point_id<<") to new format, couldn't find a matching pin number: "<<connected_component["pin_number"]<<std::endl;
                    std::cerr<<"Make sure to check and repair and check the net with id:"<<dynamic_cast<types::net*>(this->parent)->get_id()<<"!"<<std::endl;
                    return false;
                }
            } else {
                std::cerr<<"Error converting a net point("<<this->point_id<<") to new format, couldn't find component instance with id: "<<connected_component["instance_id"]<<std::endl;
                std::cerr<<"Make sure to check and repair and check the net with id:"<<dynamic_cast<types::net*>(this->parent)->get_id()<<"!"<<std::endl;
                return false;
            }
        }
    }
    return true;
}

json::object_t open_json::types::net_point::get_json() {
    json out = {
        {"connected_action_regions", json::value_t::array },
        {"point_id", this->point_id},
        {"connected_points", this->connected_point_ids},
        {"x", this->position.x},
        {"y", this->position.y}
    };
    
    for (connected_action_region region : this->connected_action_regions) {
        out["connected_action_regions"].push_back(json({
            {"action_region_index", region.action_region_index},
            {"body_index", region.body_index},
            {"instance_id", region.component_instance_id},
            {"order", region.order_index},
            {"signal", region.signal_name}
        }));
    }
    
    return out;
}

// Shapes
// Shape
void open_json::types::shapes::shape::read(json json_data) {
    this->flip = open_json::get_boolean(json_data["flip"]);
    this->rotation = open_json::get_value_or_default(json_data, "rotation", this->rotation);
    
    if (json_data.find("styles") != json_data.end()) {
        open_json::types::populate_attributes(this->styles, json_data["styles"]);
    }
}

json::object_t open_json::types::shapes::shape::get_json() {
    return {
        {"flip", this->flip},
        {"rotation", this->rotation},
        {"type", open_json::types::shapes::name_shape_type_registry[this->type]},
        {"styles", this->styles}
    };
}

// Rectangle
void open_json::types::shapes::rectangle::read(json json_data) {
    this->width = open_json::get_value_or_default(json_data, "width", this->width);
    this->height = open_json::get_value_or_default(json_data, "height", this->height);
    this->position = open_json::types::point(open_json::get_value_or_default(json_data, "x", this->position.x), open_json::get_value_or_default(json_data, "y", this->position.y));
    this->line_width = open_json::get_value_or_default(json_data, "line_width", this->line_width);
}

json::object_t open_json::types::shapes::rectangle::get_json() {
    json shape = shape::get_json();
    shape["width"] = this->width;
    shape["height"] = this->height;
    shape["x"] = this->position.x;
    shape["y"] = this->position.y;
    shape["line_width"] = this->line_width;
    return shape;
}

// Rounded Rectangle
void open_json::types::shapes::rounded_rectangle::read(json json_data) {
    this->radius = open_json::get_value_or_default(json_data, "radius", this->radius);
}

json::object_t open_json::types::shapes::rounded_rectangle::get_json() {
    json rectangle = rectangle::get_json();
    rectangle["radius"] = radius;
    return rectangle;
}

// Arc
void open_json::types::shapes::arc::read(json json_data) {
    this->is_clockwise = open_json::get_boolean(json_data["is_clockwise"], true);
    this->start_angle = open_json::get_value_or_default(json_data, "start_angle", this->start_angle);
    this->end_angle = open_json::get_value_or_default(json_data, "end_angle", this->end_angle);
    this->radius = open_json::get_value_or_default(json_data, "radius", this->radius);
    this->width = open_json::get_value_or_default(json_data, "width", this->width);
    this->position = open_json::types::point(open_json::get_value_or_default(json_data, "x", this->position.x), open_json::get_value_or_default(json_data, "y", this->position.y));
}

json::object_t open_json::types::shapes::arc::get_json() {
    json shape = shape::get_json();
    shape["is_clockwise"] = this->is_clockwise;
    shape["start_angle"] = this->start_angle;
    shape["end_angle"] = this->end_angle;
    shape["radius"] = this->radius;
    shape["width"] = this->width;
    shape["x"] = this->position.x;
    shape["y"] = this->position.y;
    return shape;
}

// Circle
void open_json::types::shapes::circle::read(json json_data) {
    this->radius = open_json::get_value_or_default(json_data, "radius", this->radius);
    this->position = open_json::types::point(open_json::get_value_or_default(json_data, "x", this->position.x), open_json::get_value_or_default(json_data, "y", this->position.y));
    this->line_width = open_json::get_value_or_default(json_data, "line_width", this->line_width);
}

json::object_t open_json::types::shapes::circle::get_json() {
    json shape = shape::get_json();
    shape["radius"] = this->radius;
    shape["x"] = this->position.x;
    shape["y"] = this->position.y;
    shape["line_width"] = this->line_width;
    return shape;
}

// Label
void open_json::types::shapes::label::read(json json_data) {
    // Default to sans serif
    this->font_family = open_json::get_value_or_default<std::string>(json_data, "font_family", "sans-serif");
    this->font_size = open_json::get_value_or_default(json_data, "font_size", this->font_size);
    this->position = open_json::types::point(open_json::get_value_or_default(json_data, "x", this->position.x), open_json::get_value_or_default(json_data, "y", this->position.y));
    this->text = open_json::get_value_or_default<std::string>(json_data, "text", "");
    
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
            this->baseline = baseline_types::ALPHABETIC;
        } else if (json_data["baseline"] == "middle") {
            this->baseline = baseline_types::MIDDLE;
        } else if (json_data["baseline"] == "hanging") {
            this->baseline = baseline_types::HANGING;
        }
    }
}

json::object_t open_json::types::shapes::label::get_json() {
    json shape = shape::get_json();
    shape["font_family"] = this->font_family;
    shape["font_size"] = this->font_size;
    shape["x"] = this->position.x;
    shape["y"] = this->position.y;
    shape["text"] = this->text;
    
    switch (this->align) {
        case alignment::RIGHT:
            shape["align"] = "right";
            break;
        case alignment::CENTER:
            shape["align"] = "center";
            break;
        case alignment::LEFT:
        default:
            shape["align"] = "left";
            break;
    }
    
    switch (this->baseline) {
        case baseline_types::MIDDLE:
            shape["baseline"] = "middle";
            break;
        case baseline_types::HANGING:
            shape["baseline"] = "hanging";
            break;
        case baseline_types::ALPHABETIC:
        default:
            shape["baseline"] = "alphabetic";
            break;
    }

    return shape;
}

// Line
void open_json::types::shapes::line::read(json json_data) {
    this->width = open_json::get_value_or_default(json_data, "width", this->width);
    
    if (json_data.find("p1") != json_data.end()) {
        this->start = open_json::types::point(open_json::get_value_or_default(json_data["p1"], "x", this->start.x), open_json::get_value_or_default(json_data["p1"], "y", this->start.y));
    }
    
    if (json_data.find("p2") != json_data.end()) {
        this->end = open_json::types::point(open_json::get_value_or_default(json_data["p2"], "x", this->end.x), open_json::get_value_or_default(json_data["p2"], "y", this->end.y));
    }
}

json::object_t open_json::types::shapes::line::get_json() {
    json shape = shape::get_json();
    shape["width"] = this->width;
    shape["p1"]["x"] = this->start.x;
    shape["p1"]["y"] = this->start.y;
    shape["p2"]["x"] = this->end.x;
    shape["p2"]["y"] = this->end.y;
    return shape;
}

// Rounded Segment
void open_json::types::shapes::rounded_segment::read(json json_data) {
    this->radius = open_json::get_value_or_default(json_data, "radius", this->radius);
}

json::object_t open_json::types::shapes::rounded_segment::get_json() {
    json line = line::get_json();
    line["radius"] = radius;
    return line;
}

// Polygon
void open_json::types::shapes::polygon::read(json json_data) {
    this->line_width = open_json::get_value_or_default(json_data, "line_width", this->line_width);
    for (json::object_t point : json_data["points"]) {
        if (point.find("x") != point.end() && point.find("y") != point.end()) {
            this->points.emplace_back(point["x"], point["y"]);
        }
    }
    for (std::string shape_type_name : json_data["shape_types"]) {
        if (open_json::types::shapes::shape_typename_registry.find(shape_type_name) == open_json::types::shapes::shape_typename_registry.end()) {
            throw parse_exception("Invalid shape type specified: " + shape_type_name + "!");
        }
        this->shape_types.push_back(open_json::types::shapes::shape_typename_registry[shape_type_name]);
    }
}

json::object_t open_json::types::shapes::polygon::get_json() {
    json shape = shape::get_json();
    shape["line_width"] = this->line_width;
    for (point &p : this->points) {
        shape["points"].push_back(json({{"x", p.x}, {"y", p.y}}));
    }
    for (shape_type t : this->shape_types) {
        shape["shape_types"].push_back(open_json::types::shapes::name_shape_type_registry[t]);
    }
    return shape;
}

// Bezier Curve
void open_json::types::shapes::bezier_curve::read(json json_data) {
    if (json_data.find("p1") != json_data.end()) {
        this->start = open_json::types::point(open_json::get_value_or_default(json_data["p1"], "x", this->start.x), open_json::get_value_or_default(json_data["p1"], "y", this->start.y));
    }
    
    if (json_data.find("p2") != json_data.end()) {
        this->end = open_json::types::point(open_json::get_value_or_default(json_data["p2"], "x", this->end.x), open_json::get_value_or_default(json_data["p2"], "y", this->end.y));
    }
    
    if (json_data.find("control1") != json_data.end()) {
        this->control_point1 = open_json::types::point(open_json::get_value_or_default(json_data["control1"], "x", this->control_point1.x), open_json::get_value_or_default(json_data["control1"], "y", this->control_point1.y));
    }
    
    if (json_data.find("control2") != json_data.end()) {
        this->control_point2 = open_json::types::point(open_json::get_value_or_default(json_data["control2"], "x", this->control_point2.x), open_json::get_value_or_default(json_data["control2"], "y", this->control_point2.y));
    }
}

json::object_t open_json::types::shapes::bezier_curve::get_json() {
    json shape = shape::get_json();
    shape["p1"]["x"] = this->start.x;
    shape["p1"]["y"] = this->start.y;
    shape["p2"]["x"] = this->end.x;
    shape["p2"]["y"] = this->end.y;
    shape["control1"]["x"] = this->control_point1.x;
    shape["control1"]["y"] = this->control_point1.y;
    shape["control2"]["x"] = this->control_point2.x;
    shape["control2"]["y"] = this->control_point2.y;
    return shape;
}

// OpenJSON
void open_json::open_json_format::read(std::vector<std::string> files) {
    for (std::string file : files) {
        std::cout<<"Parsing: "<<split(file, "/").back()<<std::endl;
        std::ifstream file_stream(file);
        json raw_json_data;
        file_stream >> raw_json_data;
        this->parsed_data.emplace_back(new data(split(split(file, "/").back(), "\\.").front(), raw_json_data));
        file_stream.close();
    }
}

void open_json::open_json_format::write(output_type type, std::string out_file) {
    // XXX This really is only useful in testing, need to better specify output file names
    for (auto data : this->parsed_data) {
        std::ofstream file_stream(data->original_file_name + out_file);
        json raw_json = data->get_json();
        file_stream << std::setw(4) << raw_json << std::endl;
        file_stream.close();
    }
}