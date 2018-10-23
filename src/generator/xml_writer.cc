#include "xml_writer.h"

namespace iris
{
    xml_attribs::xml_attribs()
    {
    }
    xml_attribs::xml_attribs(const string_piece & attr_key, const string_piece & attr_value)
    {
        add(attr_key, attr_value);
    }
    xml_attribs& xml_attribs::add(const string_piece & attr_key, const string_piece & attr_value)
    {
        push_back(std::make_pair(attr_key, attr_value));
        return *this;
    }
    xml_element_writer::xml_element_writer(
        ostream & out, string const & tag, 
        const xml_attribs & attribs)
        : xml_element_writer(out, tag, attribs, 0)
    {
    }
    xml_element_writer::xml_element_writer(
        ostream& out, string const & tag, 
        const xml_attribs& attribs, int indent)
        : m_out(out)
        , m_tag(tag)
        , m_indent(indent)
        , m_opening_tag_finished(false)
        , m_one_line(true)
    {
        m_out << std::string(indent, ' ') << '<' << tag;
        for (auto attribute : attribs)
        {
            m_out << ' ' << (string)attribute.first << "=\"" << (string)attribute.second << '"';
        }
    }
    xml_element_writer::~xml_element_writer()
    {
        if (!m_opening_tag_finished) {
            m_out << " />" << std::endl;
        } else {
            if (!m_one_line)
                m_out << std::string(m_indent, ' ');
            m_out << "</" << m_tag << '>' << std::endl;
        }
    }
    void xml_element_writer::text(const string_piece & content)
    {
        start_content(false);
        m_out << (string)content;
    }
    unique_ptr<xml_element_writer> xml_element_writer::sub_element(string const & tag)
    {
        return sub_element(tag, xml_attribs());
    }
    unique_ptr<xml_element_writer> xml_element_writer::sub_element(string const& tag, xml_attribs const & attribs)
    {
        start_content(true);
        return make_unique<xml_element_writer>(m_out, tag, attribs, m_indent + 2);
    }
    ostream& xml_element_writer::start_content(bool start_new_line)
    {
        if (!m_opening_tag_finished) {
            m_out << '>';
            m_opening_tag_finished = true;
            if (start_new_line && m_one_line) {
                m_out << endl;
                m_one_line = false;
            }
        }
        return m_out;
    }
    string xml_escape(const string & value)
    {
        string result;
        for (char c : value) {
            switch (c) {
            case '\n':
                result += "&#10;";
                break;
            case '\r':
                result += "&#13;";
                break;
            case '\t':
                result += "&#9;";
                break;
            case '"':
                result += "&quot;";
                break;
            case '<':
                result += "&lt;";
                break;
            case '>':
                result += "&gt;";
                break;
            case '&':
                result += "&amp;";
                break;
            default:
                result += c;
            }
        }
        return result;
    }
}

