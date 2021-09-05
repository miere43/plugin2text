#pragma once
#include "common.hpp"
#include "parseutils.hpp"

enum class XmlTokenType {
    // <?xml ... ?>
    StartDeclaration = 1,
    EndDeclaration,
    Attribute,
    StartElement,
    EndElement,
    Text,
};

struct XmlAttribute {
    String key;
    String value;
};

struct XmlToken {
    XmlTokenType type;
    union {
        String declaration_name;
        XmlAttribute attribute;
        String start_element_name;
        struct {
            String end_element_name;
            bool self_closing;
        };
        String text;
    };
    
    inline XmlToken() {
        memset(this, 0, sizeof(*this));
    }
    
    void print();
};

enum class XmlNodeType {
    Document,
    Element,
    Text,
};

struct XmlNode {
    XmlNodeType type;
};

struct XmlElement : public XmlNode {
    String name;
    Array<XmlNode*> children{ tmpalloc };
    Array<XmlAttribute> attributes{ tmpalloc };
    bool self_closing = false;
};

struct XmlDocument : public XmlNode {
    int child_count = 0;
    XmlElement* children[8]{ 0 };
};

struct XmlText : public XmlNode {
    String text;
};

struct XmlParser {
    char* start = 0;
    char* now = 0;
    char* end = 0;

    String last_element_tag_name;
    int element_depth = 0;
    bool inside_declaration = false;
    bool inside_element = false;

    void init(const String& source);

    bool next(XmlToken* token);
private:
    void parse_text(XmlToken* token);
    void parse_attribute(XmlToken* token);
    String parse_attribute_key();
    String parse_attribute_value();
    void parse_declaration_tag(XmlToken* token);
    bool parse_element_tag(XmlToken* token);
    void skip_whitespace();
    void skip_whitespace_and_newlines();
};

XmlElement* parse_xml(const String& source);

struct XmlFormatter {
    Slice slice;
    int indent = 0;

    String format(const String& source);
private:
    void write_indent();
    void format_node(const XmlNode* node);
};