#ifndef SETBASE_H
#define SETBASE_H

#include <set>
#include <map>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <tuple>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include "pugixml.hpp"
#include "base_typeconv.h"
#include "base_mutex.h"
#define XMLKEY_ROOT "config" ///< The define of ROOT node

#ifdef WIN32
#pragma warning(disable : 4250)
#endif

using namespace pugi;
using namespace BASE;

namespace SET
{
    enum class modeofmeanpole
    {
        linear,
        cubic
        //cmp2015
    };

    /**
    * @brief The base class for xml std::set
    */
    class SET_BASE : public pugi::xml_node
    {
    public:
        /// defalut constructor
        SET_BASE();
        /// defalut destructor
        virtual ~SET_BASE();

        /**
        * @brief  read configuration from file
        * @param[in] file file name
        */
        int read(const std::string& file);

        /**
        * @brief  read configuration from istream
        * @param[in] is istream name
        */
        int read_istream(std::istream& is);

        /**
        * @brief  write configuration in file
        * @param[in] file file name
        */
        int write(const std::string& file);

        /**
        * @brief  write configuration in std::ostream
        * @param[in] os istream name
        */
        int write_ostream(std::ostream& os);

        /**
        * @brief  std::set/get glog pointer (VIRTUAL for gsetout!)
        * @param[in] l The pointer of log file
        */

        /**
         * @brief return the glog pointer (VIRTUAL for gsetout!)
         * @return glog pointer
         */

         /// application name
        virtual std::string pgm();

        /// application info/version
        virtual std::string app();

        /**
        * @brief  get some information of the application
        * @param[out] pgm program name
        * @param[out] ver program version
        * @param[out] dat program running data
        * @param[out] tim program running time
        * @param[out] rev program revision number
        * @param[out] own program owner name
        */
        virtual void app(const std::string& pgm, const std::string& ver,
            const std::string& dat, const std::string& tim,
            const std::string& rev, const std::string& own);

        /// application usage
        virtual void usage();

        /**
        * @brief  get command line arguments
        * @param[in] argc   argument number
        * @param[in] argv[] command line array of arguments
        */
        virtual void arg(int argc, char* argv[],
            bool add = false, bool thin = false);

        /// settings help
        virtual void help() = 0;

        /// settings check
        virtual void check() = 0;

        /**
         * @brief get all types of actual settings
         * @return std::set<std::string> : all types of actual settings
         */
        std::set<std::string> types() { return _set; }

        /**
         * @brief public interface
         * @param[in] elem  element name in xml
         * @param[in] subelem  sub-element name in xml
         * @return std::string : The value of elem.sub-element
         */
        std::string strval(const std::string& elem, const std::string& subelem);

        /**
         * @brief public interface
         * @param[in] elem  element name in xml
         * @param[in] subelem  sub-element name in xml
         * @return std::set<std::string> : The value of elem.sub-element
         */
        std::set<std::string> setval(const std::string& elem, const std::string& subelem);

        std::set<std::string> setvals(const std::string& elem, const std::string& subelem); //added by yjqin,for leos

        std::map<std::string, std::set<std::string>> setval_map(const std::string& elem, const std::string& subelem, const std::string& subelem1, const char* attri);

        void setval(const std::string& elem, const std::string& subelem, const std::string& val);     // modify the setting (added by zhshen)
        void setval(const std::string& elem, const std::string& subelem, const std::string& sub_subelem, const std::string& val);
        /**
         * @brief public interface
         * @param[in] elem  element name in xml
         * @param[in] subelem  sub-element name in xml
         * @return std::vector<std::string> : The value of elem.sub-element
         */
        std::vector<std::string> vecval(const std::string& elem, const std::string& subelem);

        /// thin execution
        bool thin();

        /// output the log cache

        /// get the node of confige
        xml_node xml_root_node();
        xml_node xml_config_node(std::string node_name);

        // jdhuang : return xml file name
        std::string xml_name() { return _name; };

        /**
        * @brief remove spaces, tabs and newlines in the std::string
        * @param[in]  str    std::string
        */
        void str_erase(std::string& str); //hyChang : used for removing space, tab and newline

    protected:
        /**
         * @brief get the double value of ROOT.elem.attrib
         * @param[in] elem    element name in xml
         * @param[in] attrib  attribute name in xml
         * @return double : The value of ROOT.elem.attribute
         */
        double _dblatt(const std::string& elem, const std::string& attrib);

        /**
         * @brief get the double value of ROOT.elem.subelem
         * @param[in] elem    element name in xml
         * @param[in] subelem subelem name in xml
         * @return double : The value of ROOT.elem.subelem
         */
        double _dblval(const std::string& elem, const std::string& subelem);

        /**
        * @brief get the std::string value of ROOT.elem.subelem
        * @param[in] elem    element name in xml
        * @param[in] subelem subelem name in xml
        * @return std::string : The value of ROOT.elem.subelem
        */
        std::string _strval(const std::string& elem, const std::string& subelem);

        /**
        * @brief get the std::set<std::string> value of ROOT.elem.subelem
        * @param[in] elem    element name in xml
        * @param[in] subelem subelem name in xml
        * @return std::set<std::string> : The value of ROOT.elem.subelem
        */
        std::set<std::string> _setval(const std::string& elem, const std::string& subelem);
        std::set<std::string> _setvals(const std::string& elem, const std::string& subelem);                            //added by yjqin,for leos
        std::set<std::string> _setvals(const std::string& elem, const std::string& subelem, const std::string& sub_subelem); //added by Hwang Shih

        void _setval(const std::string& elem, const std::string& subelem, const std::string& val);     // modify the setting (added by zhshen)
        void _setval(const std::string& elem, const std::string& subelem, const std::string& sub_subelem, const std::string& val); // modify the setting (added by zhshen)

        std::map<std::string, std::set<std::string>> _setval_map(const std::string& elem, const std::string& subelem, const std::string& subelem1, const char* attri);

        std::set<std::string> _setval_attribute(const std::string& elem, const std::string& subelem, const std::string& attri_name, const std::string& attri_value);
        /**
        * @brief get the std::vector<std::string> value of ROOT.elem.subelem
        * @param[in] elem    element name in xml
        * @param[in] subelem subelem name in xml
        * @return std::vector<std::string> : The value of ROOT.elem.subelem
        */
        std::vector<std::string> _vecval(const std::string& elem, const std::string& subelem);

        /**
        * @brief get default child node of a xml node
        * @param[in] node    the parent node
        * @param[in] n       child node name
        * @param[in] val     child node value
        * @param[in] reset   reset or not
        * @return xml_node : The child node
        */
        xml_node _default_node(xml_node& node, const char* n, const char* val = "", bool reset = false);

        /**
        * @brief get default node of a xml node by the attribute or reset it with v
        * @param[in] node    the parent node
        * @param[in] n       attribute name
        * @param[in] v       reset value
        * @param[in] reset   reset or not
        * @return xml_node : The child node
        */
        void _default_attr(xml_node& node, const char* n, const std::string& v, bool reset = false);

        /**
        * @brief get default node of a xml node by the attribute or reset it with v
        * @param[in] node    the parent node
        * @param[in] n       attribute name
        * @param[in] v       reset value
        * @param[in] reset   reset or not
        * @return xml_node : The child node
        */
        void _default_attr(xml_node& node, const char* n, const bool& value, bool reset = false);

        /**
        * @brief get default node of a xml node by the attribute or reset it with v
        * @param[in] node    the parent node
        * @param[in] n       attribute name
        * @param[in] v       reset value
        * @param[in] reset   reset or not
        * @return xml_node : The child node
        */
        void _default_attr(xml_node& node, const char* n, const int& value, bool reset = false);

        /**
        * @brief get default node of a xml node by the attribute or reset it with v
        * @param[in] node    the parent node
        * @param[in] n       attribute name
        * @param[in] v       reset value
        * @param[in] reset   reset or not
        * @return xml_node : The child node
        */
        void _default_attr(xml_node& node, const char* n, const double& value, bool reset = false);

        /// report element issues
        void _add_log(std::string element, std::string msg);

        /// update glog (mask,verb)
        virtual void _upd_glog() {};

        /// XML settings header
        virtual void help_header();

        /// XML settings footer
        virtual void help_footer();

        xml_document _doc;     ///< root document
        xml_parse_result _irc; ///< result status

    protected:
        std::string _name;      ///< configuration name
        std::string _delimiter; ///< delimiter for writting nodes/elements

        std::string _pgm; ///< program name
        std::string _ver; ///< program version
        std::string _rev; ///< program revision
        std::string _own; ///< program owner
        std::string _dat; ///< program data
        std::string _tim; ///< program time

        std::set<std::string> _set;                     ///< program settings
        std::map<std::string, std::set<std::string>> _chache_log; ///< The chache of log pointer

        mutable BASE_MUTEX _gmutex; ///< For the thread

    private:
    };

} // namespace

#endif
