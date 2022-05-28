#pragma once

// system includes
#include <string>

// 3rdparty lib includes
#include <LiquidCrystal_I2C.h>

class LCD : public LiquidCrystal_I2C
{
    using Base = LiquidCrystal_I2C;
public:
    using Base::Base;
    // print complete line
    size_t printcln(const char *str);
    size_t printcln(const std::string& str);
};

void display_reconnect_message();