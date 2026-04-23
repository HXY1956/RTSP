#ifndef BASE_XML_H
#define BASE_XML_H

#include <set>
#include <cstring>
#include <ctype.h>
#include <iostream>
#include <vector>
#include "base_mutex.h"
#include "base_typeconv.h"
#define PUGIXML_HEADER_ONLY
#include "pugixml.hpp"

namespace BASE
{
    class BASE_XML : public pugi::xml_node
    {
    public:
        /** @brief constructor 1. */
        explicit BASE_XML(std::string s, bool upper = true);

        /** @brief default destructor. */
        virtual ~BASE_XML();

        /** @brief read from file. */
        int read(const std::string& file);

        /** @brief read from istream. */
        int read_istream(std::istream& is);

        /** @brief write in file. */
        int write(const std::string& file);

        /** @brief write in std::ostream. */
        int write_ostream(std::ostream& os);

        std::string root() { return _root; }

        /** @brief settings check. */
        virtual void check() {};

        /** @brief settings help. */
        virtual void help() {};

        std::string strval(const std::string& elem, const std::string& subelem);         // public interface
        std::set<std::string> setval(const std::string& elem, const std::string& subelem);    // public interface
        std::vector<std::string> vecval(const std::string& elem, const std::string& subelem); // public interface

    protected:
        std::string _strval(const std::string& elem, const std::string& subelem);
        std::set<std::string> _setval(const std::string& elem, const std::string& subelem);
        std::vector<std::string> _vecval(const std::string& elem, const std::string& subelem);

        xml_node _default_node(xml_node& node, const char* n, const char* val = "", bool reset = false);

        void _default_attr(xml_node& node, const char* n, const std::string& val, bool reset = false);
        void _default_attr(xml_node& node, const char* n, const bool& val, bool reset = false);
        void _default_attr(xml_node& node, const char* n, const int& val, bool reset = false);
        void _default_attr(xml_node& node, const char* n, const double& val, bool reset = false);

        virtual void help_header(); ///< XML settings header
        virtual void help_footer(); ///< XML settings footer

        pugi::xml_document _doc;     ///< root document
        pugi::xml_parse_result _irc; ///< result status

        std::string _name;      ///< file name
        std::string _root;      ///< root directory
        std::string _delimiter; ///< delimiter for writting nodes/elements
        bool _ucase;       ///< upper/lower case for keywords
        BASE_MUTEX _gmutexxml;
#ifdef BMUTEX
        boost::mutex _mutex; ///< muttable access
#endif

    private:
    };

}

#endif
