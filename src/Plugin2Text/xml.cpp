#include "xml.hpp"
#include "string.hpp"
#include "array.hpp"
#include <stdlib.h> // _countof
#include <string.h> // memcpy

inline static bool is_whitespace(char c) {
    return c == ' ' || c == '\t';
}

inline static bool is_newline(char c) {
    return c == '\r' || c == '\n';
}

void XmlParser::init(const String& source) {
    start = source.chars;
    now = start;
    end = source.chars + source.count;
}

bool XmlParser::next(XmlToken* token) {
    bool skipped_text_whitespace = false;
    while (true) {
        if (inside_declaration) {
            parse_declaration_tag(token);
            return true;
        } else if (inside_element) {
            if (parse_element_tag(token)) {
                continue;
            }
            return true;
        }

        if (element_depth <= 0) {
            skip_whitespace_and_newlines();
        }

        if (now >= end) {
            return false;
        }

        char c = *now;
        if (c == '<') {
            ++now;
            verify(now < end);
            c = *now;
            if (c == '?') {
                ++now;
                verify(now < end);

                inside_declaration = true;
                token->type = XmlTokenType::StartDeclaration;
                token->declaration_name = parse_attribute_key();
                return true;
            } else if (c == '!') {
                // Skip !DOCTYPE
                ++now;
                do {
                    verify(now < end);
                    c = *now++;
                    if (c == '>') {
                        break;
                    }
                } while (true);
                continue;
            } else if (c == '/') {
                ++now;
                verify(now < end);
                String name = parse_attribute_key();
                verify(now < end);
                verify(*now == '>');
                ++now;
                token->type = XmlTokenType::EndElement;
                token->end_element_name = name;
                token->self_closing = false;
                --element_depth;
                verify(element_depth >= 0);
                return true;
            } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '-' || c == ':') {
                inside_element = true;
                token->type = XmlTokenType::StartElement;
                token->start_element_name = parse_attribute_key();
                last_element_tag_name = token->start_element_name;
                ++element_depth;
                return true;
            } else {
                verify(false);
            }
        } else {
            verify(!skipped_text_whitespace);
            if (element_depth <= 0) {
                skip_whitespace_and_newlines();
                skipped_text_whitespace = true;
                continue; // Scan next token.
            } else {
                parse_text(token);
                return true;
            }
        }

        verify(false);
        return false;
    }
}

void XmlParser::parse_text(XmlToken* token) {
    char* start = now;
    while (now < end) {
        char c = *now;
        // @TODO: c == special character, not just '<'.
        if (c == '<') {
            break;
        } else {
            ++now;
        }
    }
    int count = (int)(now - start);
    verify(count > 0);
    token->type = XmlTokenType::Text;
    token->text = { start, count };
}

void XmlParser::parse_attribute(XmlToken* token) {
    auto key = parse_attribute_key();
    skip_whitespace();
    verify(now < end);
    verify(*now == '=');
    ++now;
    skip_whitespace();
    auto value = parse_attribute_value();

    token->type = XmlTokenType::Attribute;
    token->attribute.key = key;
    token->attribute.value = value;
}

String XmlParser::parse_attribute_key() {
    char* start = now;
    while (now < end) {
        char c = *now;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == ':') {
            ++now;
        } else {
            break;
        }
    }
    int count = (int)(now - start);
    verify(count > 0);
    return { start, count };
}

String XmlParser::parse_attribute_value() {
    auto start = now;
    verify(now < end);
    char quote_char = 0;
    char c = *now;
    if (c == '"' || c == '\'') {
        quote_char = c;
        ++start;
        ++now;
    }

    while (now < end) {
        char c = *now;
        if (quote_char ? c == quote_char : is_whitespace(c)) {
            break;
        } else if (is_newline(c)) {
            verify(false);
        } else {
            ++now;
        }
    }
    int count = (int)(now - start);
    verify(quote_char || count > 0);
    if (quote_char) ++now;

    return { start, count };
}

void XmlParser::parse_declaration_tag(XmlToken* token) {
    skip_whitespace();
    verify(now < end);
    char c = *now;
    if (c == '?') {
        ++now;
        verify(now < end);
        c = *now;
        if (c == '>') {
            ++now;
            token->type = XmlTokenType::EndDeclaration;
            inside_declaration = false;
        } else {
            verify(false); // Expected '>' after '?' (end of declaration)
        }
    } else {
        parse_attribute(token);
    }
}

bool XmlParser::parse_element_tag(XmlToken* token) {
    skip_whitespace();
    verify(now < end);
    char c = *now;
    bool self_closing = c == '/';
    if (self_closing) {
        ++now;
        verify(now < end);
        c = *now;
    }

    if (c == '>') {
        ++now;
        inside_element = false;
        if (self_closing) {
            token->type = XmlTokenType::EndElement;
            token->end_element_name = last_element_tag_name;
            token->self_closing = self_closing;
            --element_depth;
            verify(element_depth >= 0);
        } else {
            return true;
        }
    } else {
        verify(!self_closing);
        parse_attribute(token);
    }
    return false;
}

void XmlParser::skip_whitespace() {
    while (now < end) {
        if (is_whitespace(*now)) {
            ++now;
        } else {
            break;
        }
    }
}

void XmlParser::skip_whitespace_and_newlines() {
    while (now < end) {
        char c = *now;
        if (is_whitespace(c) || is_newline(c)) {
            ++now;
        } else {
            break;
        }
    }
}

XmlText* parse_text(XmlParser& parser, const XmlToken& thisTextToken) {
    auto result = memnew(tmpalloc) XmlText();
    result->type = XmlNodeType::Text;
    result->text = thisTextToken.text;
    return result;
}

XmlElement* parse_element(XmlParser& parser, const XmlToken& thisElementToken) {
    XmlToken token;
    auto element = memnew(tmpalloc) XmlElement();
    element->type = XmlNodeType::Element;
    element->name = thisElementToken.start_element_name;

    // Attributes
    while (parser.next(&token)) {
        switch (token.type) {
            case XmlTokenType::Attribute: {
                element->attributes.push(token.attribute);
            } break;
            default: {
                goto finish_attributes;
            }
        }
    }

    finish_attributes:

    do {
        switch (token.type) {
            case XmlTokenType::EndElement: {
                verify(thisElementToken.start_element_name == token.end_element_name);
                element->self_closing = token.self_closing;
                return element;
            }
            case XmlTokenType::StartElement: {
                element->children.push(parse_element(parser, token));
            } break;
            case XmlTokenType::Text: {
                element->children.push(parse_text(parser, token));
            } break;
            default: {
                verify(false);
            } break;
        }
    } while (parser.next(&token));

    verify(false);
    return nullptr;
}

XmlElement* parse_xml(const String& source) {
    XmlToken token;
    XmlParser parser;
    parser.init(source);

    XmlDocument doc;
    doc.type = XmlNodeType::Document;

    // Skip declaration first.
    {
        bool inside_declaration = false;
        while (parser.next(&token)) {
            switch (token.type) {
                case XmlTokenType::StartDeclaration: {
                    inside_declaration = true;
                } break;
                case XmlTokenType::EndDeclaration: {
                    verify(inside_declaration);
                    goto declaration_finished;
                } break;
                case XmlTokenType::Attribute: {
                    verify(inside_declaration);
                } break;
                default: {
                    verify(!inside_declaration);
                    // Missing declaration.
                    goto continue_with_last_token;
                } break;
            }
        }
    }

    declaration_finished: {}

    while (parser.next(&token)) {
        continue_with_last_token: {}
        switch (token.type) {
            case XmlTokenType::StartElement: {
                verify(doc.child_count < _countof(doc.children));
                doc.children[doc.child_count++] = parse_element(parser, token);
            } break;
            default: {
                verify(false);
            } break;
        }
    }

    // We can parse multiple elements at root level, but for now we don't need it,
    // so we return first element.
    verify(doc.child_count == 1);
    return doc.children[0];
}

String XmlFormatter::format(const String& source) {
    *this = XmlFormatter();

    const auto root = parse_xml(source);

    slice.start = tmpalloc.now;
    slice.now = slice.start;
    slice.end = slice.start + tmpalloc.remaining_size();

    format_node(root);

    tmpalloc.now = slice.now;
    slice.end = slice.now;
    return { (char*)slice.start, (int)slice.size() };
}

void XmlFormatter::write_indent() {
    for (int i = 0; i < indent; ++i) {
        slice.write_literal("  ");
    }
}

void XmlFormatter::format_node(const XmlNode* node) {
    switch (node->type) {
        case XmlNodeType::Element: {
            const auto element = (const XmlElement*)node;
            write_indent();
            slice.write_literal("<");
            slice.write_string(element->name);

            for (const auto& attribute : element->attributes) {
                slice.write_literal(" ");
                slice.write_string(attribute.key);
                slice.write_literal("=\"");
                slice.write_string(attribute.value);
                slice.write_literal("\"");
            }

            if (element->self_closing) {
                slice.write_literal(" />");
                break;
            }

            slice.write_literal(">");
            
            ++indent;
            if (element->children.count == 0) {
                --indent;
            } else if (element->children.count == 1 && element->children[0]->type == XmlNodeType::Text) {
                const auto text = (const XmlText*)element->children[0];
                slice.write_string(text->text);

                --indent;
            } else {
                slice.write_literal("\n");
                for (int i = 0; i < element->children.count; ++i) {
                    const auto child = element->children[i];
                    format_node(child);
                    
                    if (i != element->children.count - 1) {
                        slice.write_literal("\n");
                    }
                }

                --indent;
                slice.write_literal("\n");
                write_indent();
            }

            slice.write_literal("</");
            slice.write_string(element->name);
            slice.write_literal(">");
        } break;

        case XmlNodeType::Text: {
            const auto text = (const XmlText*)node;
            write_indent();
            slice.write_string(text->text);
        } break;

        default: {
            verify(false);
        } break;
    }
}
