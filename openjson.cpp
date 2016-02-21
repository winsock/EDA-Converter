#include "converter.hpp"
#include "openjson.hpp"

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
    
    design_info.read(json_data);
    
    if (json_data.find("components") != json_data.end()) {
        json componentsJson = json_data["components"];
        for (json::iterator it = componentsJson.begin(); it != componentsJson.end(); it++) {
            std::shared_ptr<types::component> component(new types::component(it.key()));
            component->read(it.value());
            this->components[it.key()] = component;
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
            std::shared_ptr<types::component_instance> componentInstance(new types::component_instance(this->components[json_object["library_id"]]));
            componentInstance->read(json_object);
            this->component_instances[componentInstance->get_id()] = componentInstance;
        }
    }
    
    if (json_data.find("layer_options") != json_data.end()) {
        json::array_t options = json_data["layer_options"];
        for (json::object_t json_object : options) {
            std::shared_ptr<types::layer_option> option(new types::layer_option());
            option->read(json_object);
            this->layer_options.push_back(option);
        }
    }
    
    if (json_data.find("layout_bodies") != json_data.end()) {
        json::array_t bodies = json_data["layout_bodies"];
        for (json::object_t json_object : bodies) {
            std::shared_ptr<types::body> body(new types::body());
            body->read(json_object);
            this->layout_bodies.push_back(body);
        }
    }
    
    if (json_data.find("layout_body_attributes") != json_data.end()) {
        json::array_t body_attributes = json_data["layout_body_attributes"];
        for (json::object_t json_object : body_attributes) {
            std::shared_ptr<types::layout_body_attribute> body_attribute(new types::layout_body_attribute());
            body_attribute->read(json_object);
            this->layout_body_attributes.push_back(body_attribute);
        }
    }
    
    if (json_data.find("layout_objects") != json_data.end()) {
        json::array_t layout_objects = json_data["layout_objects"];
        for (json::object_t json_object : layout_objects) {
            std::shared_ptr<types::layout_object> layout_object(new types::layout_object());
            layout_object->read(json_object);
            this->layout_objects.push_back(layout_object);
        }
    }
    
    if (json_data.find("nets") != json_data.end()) {
        json::array_t nets = json_data["nets"];
        for (json::object_t json_object : nets) {
            if (json_object.find("net_id") == json_object.end()) {
                throw new parse_exception("Net has no net id!");
            }
            std::shared_ptr<types::net> net(new types::net());
            net->read(json_object);
            this->nets[net->get_id()] = net;
        }
    }
    
    if (json_data.find("pcb_text") != json_data.end()) {
        json::array_t pcb_text_array = json_data["pcb_text"];
        for (json::object_t json_object : pcb_text_array) {
            std::shared_ptr<types::pcb_text> pcb_text(new types::pcb_text());
            pcb_text->read(json_object);
            this->pcb_text.push_back(pcb_text);
        }
    }
    
    if (json_data.find("pours") != json_data.end()) {
        json::array_t pours = json_data["pours"];
        for (json::object_t json_object : pours) {
            std::shared_ptr<types::pour> pour(new types::pour());
            pour->read(json_object);
            this->pours.push_back(pour);
        }
    }
    
    if (json_data.find("trace_segments") != json_data.end()) {
        json::array_t traces = json_data["trace_segments"];
        for (json::object_t json_object : traces) {
            std::shared_ptr<types::trace> trace(new types::trace());
            trace->read(json_object);
            this->traces.push_back(trace);
        }
    }
}

void open_json::data::write(json json) {
    
}