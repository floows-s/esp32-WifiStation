/*
    Created-by: Floris Vrij (floows-s)
    Purpose: Defining statuses of a WifiStation.
*/

#pragma once

enum wifi_station_status {

    DISCONNECTED, // Disconnected from wifi
    CONNECTED, // Connected to wifi

    STARTING_DISCONNECTION, // User started disconnection from wifi
    STARTING_CONNECTION, // Starting connection to wifi

    RECONNECTING, // Lost wifi connection and reconnecting
};
