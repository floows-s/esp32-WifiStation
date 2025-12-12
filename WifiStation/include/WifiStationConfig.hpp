/*
    Created-by: Floris Vrij (floows-s)
    Purpose: Grouping configuration data for a WifiStation.
*/

#pragma once

#include <string>

struct WifiStationConfig{
    std::string ssid;
    std::string password;
    bool auto_reconnect = true;

    int max_con_failures = 10; // Note: When set to -1, it will try to reconnect forever
};