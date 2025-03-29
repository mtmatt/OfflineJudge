#ifndef COLOR_H
#define COLOR_H

#include <iostream>
#include <string>
#include <sstream>

#define RESET "\e[0m"

struct Color {
    short red, green, blue;
    Color(int r = 0, int g = 0, int b = 0)
    : red(r), green(g), blue(b) {}
    std::string get() const;
};

std::ostream& operator<<(std::ostream &out, const Color &color);

inline std::string Color::get() const {
    std::stringstream ss;
    ss << (*this);
    std::string ret;
    ss >> ret;
    return ret;
}

inline std::ostream& operator<<(std::ostream &out, const Color &color) {
    out << "\e[38;2;" << color.red << ";" << color.green << ";" << color.blue << "m";
    return out;
}

#endif