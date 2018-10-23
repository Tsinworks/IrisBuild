#pragma once
#include "stl.hpp"
#include <iostream>
#include <ext/string_piece.hpp>
namespace iris
{
    using std::ext::string_piece;

    class xml_attribs : public vector<pair<string_piece, string_piece> >
    {
    public:
        xml_attribs();
        xml_attribs(const string_piece& attr_key,
            const string_piece& attr_value);
        xml_attribs& add(const string_piece& attr_key,
            const string_piece& attr_value);
    };
    class xml_element_writer
    {
    public:
        xml_element_writer(ostream& out, string const& tag, const xml_attribs& attribs);
        xml_element_writer(ostream& out, string const& tag, const xml_attribs& attribs, int indent);
        ~xml_element_writer();
        void text(const string_piece& content);
        unique_ptr<xml_element_writer> sub_element(string const& tag);
        unique_ptr<xml_element_writer> sub_element(string const& tag, xml_attribs const& attribs);
        ostream& start_content(bool start_new_line);
    private:
        ostream&    m_out;
        string      m_tag;
        int         m_indent;
        bool        m_one_line;
        bool        m_opening_tag_finished;
    };
    string xml_escape(const string& value);
}
