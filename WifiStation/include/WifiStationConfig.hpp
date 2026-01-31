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

    int max_con_failures = -1; // Note: When set to -1, it will try to reconnect forever
    int8_t max_tx_power = CONFIG_ESP_PHY_MAX_WIFI_TX_POWER * 4; // Note: 0.25 dBm per power unit
};