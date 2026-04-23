#ifndef BASE_TYPECONV_H
#define BASE_TYPECONV_H
#include <cmath>
#include <string>
#include <iomanip>
#include <iostream>

namespace BASE
{
    /** @brief double equivalence according to machine capability. */
    bool double_eq(const double& a, const double& b);

    /** @brief float equivalence according to machine capability. */
    bool float_eq(const float& a, const float& b);

    /** @brief round double. */
    double dround(double d);

    class BASE_TYPE_CONV
    {
    public:
        /** @brief double to std::string conversion (width/digits by iomanip can be added !!!) */
        static std::string dbl2str(const double&, int p = 3);
        /** @brief std::string to double conversion (avoiding blanks) */
        static double str2dbl(const std::string&);
        static double strSci2dbl(const std::string&);
        bool str2bool(const std::string&);                    // std::string type to bool, jdhuang
        static std::string int2str(const int&);                      // integer to std::string conversion (widtht can be added !!!)
        static std::string int2str(const int& num, const int& width); // integer to std::string conversion (have width)
        static int str2int(const std::string&);                      // std::string to integer conversion (avoiding blanks)
        static std::string bl2str(const bool&);
        static std::size_t substitute(std::string& line, const std::string& a, const std::string& b, bool caseSensitive = true);
        static std::string ltrim(const std::string&); // return trimmed std::string leading spaces
        static  std::string rtrim(const std::string&); // return trimmed std::string trailing spaces
        static std::string trim(const std::string&);  // returm trimmed std::string leading and trailing spaces

        static  double frac(double x);
        static std::string FloatWithoutZero(double x, int i = 3);
    };
} 

#endif