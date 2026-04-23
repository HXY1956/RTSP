#include <iomanip>
#include <sstream>
#include <fstream>
#include <algorithm>
#include "base_xml.h"

namespace BASE
{
    BASE_XML::BASE_XML(std::string s, bool upper)
    {
        _root = s;
        _name = "";
        _delimiter = "  "; // only for nodes/elements
        _ucase = true;
    }

    BASE_XML::~BASE_XML()
    {
    }

    // Read from file
    // ----------
    int BASE_XML::read(const std::string& file)
    {
        _gmutexxml.lock();

        _name = file;

        if (!(_irc = _doc.load_file(_name.c_str())))
        {
            _gmutexxml.unlock();
            return -1;
        }

        _gmutexxml.unlock();
        this->check();

        return 0;
    }

    // Read from istream
    // ----------
    int BASE_XML::read_istream(std::istream& is)
    {
        _gmutexxml.lock();

        if (!(_irc = _doc.load(is)))
        {
            _gmutexxml.unlock();
            return -1;
        }
        _gmutexxml.unlock();
        this->check();
        return 0;
    }

    // Write to file
    // ----------
    int BASE_XML::write(const std::string& file)
    {
        _gmutexxml.lock();

        std::ofstream of;
        std::string name(file); // if( name.empty() ) name = _name;  // ???
        try
        {
            of.open(name.c_str());
        }
        catch (std::fstream::failure e)
        {
            std::cerr << "Warning: Exception opening file " << name << ": " << e.what() << std::endl;
            _gmutexxml.unlock();
            return -1;
        }

        _doc.save_file(name.c_str(), _delimiter.c_str(), pugi::format_indent);
        of.close();
        _gmutexxml.unlock();
        return 0;
    }

    // Write to std::ostream
    // ----------
    int BASE_XML::write_ostream(std::ostream& os)
    {
#ifdef BMUTEX
        boost::mutex::scoped_lock lock(_mutex);
#endif
        _gmutexxml.lock();

        _doc.save(os, _delimiter.c_str(), pugi::format_indent);

        _gmutexxml.unlock();
        return 0;
    }

    // Return first std::string of value based on key
    // ----------
    std::string BASE_XML::strval(const std::string& elem, const std::string& subelem)
    {
#ifdef BMUTEX
        boost::mutex::scoped_lock lock(_mutex);
#endif
        _gmutexxml.lock();
        std::string tmp = _strval(elem, subelem);
        _gmutexxml.unlock();
        return tmp;
    }

    // Return std::set of value based on key
    // ----------
    std::set<std::string> BASE_XML::setval(const std::string& elem, const std::string& subelem)
    {
#ifdef BMUTEX
        boost::mutex::scoped_lock lock(_mutex);
#endif
        _gmutexxml.lock();

        std::set<std::string> tmp = _setval(elem, subelem);
        _gmutexxml.unlock();
        return tmp;
    }

    // Return std::vector of value based on key
    // ----------
    std::vector<std::string> BASE_XML::vecval(const std::string& elem, const std::string& subelem)
    {
#ifdef BMUTEX
        boost::mutex::scoped_lock lock(_mutex);
#endif
        _gmutexxml.lock();

        std::vector<std::string> tmp = _vecval(elem, subelem);
        _gmutexxml.unlock();
        return tmp;
    }

    // Return first std::string of value based on key
    // ----------
    std::string BASE_XML::_strval(const std::string& elem, const std::string& subelem)
    {
        std::string word;

        std::istringstream is(_doc.child(_root.c_str()).child(elem.c_str()).child_value(subelem.c_str()));

        if (is >> word)
            return word;
        return "";
    }

    // Return std::set of value based on key
    // ----------
    std::set<std::string> BASE_XML::_setval(const std::string& elem, const std::string& subelem)
    {
        std::set<std::string> vals;
        std::string word;

        std::istringstream is(_doc.child(_root.c_str()).child(elem.c_str()).child_value(subelem.c_str()));
        while (is >> word)
            vals.insert(word);

        return vals;
    }

    // Return std::vector of value based on key
    // ----------
    std::vector<std::string> BASE_XML::_vecval(const std::string& elem, const std::string& subelem)
    {
        std::vector<std::string> vec;
        std::string str;

        std::istringstream is(_doc.child(_root.c_str()).child(elem.c_str()).child_value(subelem.c_str()));
        while (is >> str)
            vec.push_back(str);

        return vec;
    }

    // check/std::set node
    // ----------
    pugi::xml_node BASE_XML::_default_node(xml_node& node, const char* n, const char* val, bool reset)
    {
#ifdef DEBUG
        std::cout << "creating node [" << n << "]  for parent [" << node.name() << "]  std::set with value [" << val << "]\n";
#endif

        std::string s(n);

        //  if( _ucase )  transform(s.begin(), s.end(), s.begin(), ::toupper);
        //  else          transform(s.begin(), s.end(), s.begin(), ::tolower);

        // remove if to be reset
        //  if( strcmp(val,"") && reset ) node.remove_child(s.c_str());
        if (reset)
            node.remove_child(s.c_str());

#ifdef DEBUG
        if (!node)
            std::cerr << " NODE[" << node.name() << "] " << node << " does'not exists !\n";
        else
            std::cerr << " NODE[" << node.name() << "] " << node << " does exists .... ok \n";
#endif

        xml_node elem = node.child(s.c_str());

#ifdef DEBUG
        if (!elem)
            std::cerr << " ELEM[" << s << "] " << elem << " does'not exists !\n";
        else
            std::cerr << " ELEM[" << s << "] " << elem << " does exists .... ok \n";
#endif

        if (!elem)
            elem = node.append_child(s.c_str());

        //  if( elem && strcmp(val,"") && reset ){
        if (elem && (strcmp(val, "") || reset))
        {
            elem.append_child(pugi::node_pcdata).set_value(val);
        }

        return elem;
    }

    // check attributes & std::set default
    // ----------
    void BASE_XML::_default_attr(xml_node& node, const char* n, const std::string& val, bool reset)
    {
        if (node.attribute(n).empty())
            node.append_attribute(n);
        if (strlen(node.attribute(n).value()) == 0 || reset)
            node.attribute(n).set_value(val.c_str());

        std::string s = node.attribute(n).value();
        //  if( _ucase ) transform(s.begin(), s.end(), s.begin(), ::toupper);
        //  else         transform(s.begin(), s.end(), s.begin(), ::tolower);
        node.attribute(n).set_value(s.c_str());

        //  std::cerr << " strcmp(val,) " << strcmp(val.c_str(),"")
        //       << " reset:" << reset << " val:" << val << "." << std::endl;
    }

    // check attributes & std::set default
    // ----------
    void BASE_XML::_default_attr(xml_node& node, const char* n, const bool& val, bool reset)
    {
        if (node.attribute(n).empty())
            node.append_attribute(n);
        if (strlen(node.attribute(n).value()) == 0 || reset)
            node.attribute(n).set_value(val);
    }

    // check attributes & std::set default
    // ----------
    void BASE_XML::_default_attr(xml_node& node, const char* n, const int& val, bool reset)
    {
        if (node.attribute(n).empty())
            node.append_attribute(n);
        if (strlen(node.attribute(n).value()) == 0 || reset)
            node.attribute(n).set_value(val);
    }

    // check attributes & std::set default
    // ----------
    void BASE_XML::_default_attr(xml_node& node, const char* n, const double& val, bool reset)
    {
        if (node.attribute(n).empty())
            node.append_attribute(n);
        if (strlen(node.attribute(n).value()) == 0 || reset)
            node.attribute(n).set_value(val);
    }

    // XML help header (optional text comment)
    // ----------
    void BASE_XML::help_header()
    {
        std::cerr << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?> \n"
            << "<!DOCTYPE " << _root << ">\n\n"
            << "<" << _root << ">\n";
    }

    // XML help footer (optional text comment)
    // ----------
    void BASE_XML::help_footer()
    {
        std::cerr << "</" << _root << ">\n";
    }

} // namespace
