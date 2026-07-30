// Mounter.hpp
//
#pragma once


#include <string>

class Disc;

class Mounter
{
public:
    static Disc mount( const std::string &filename );

};