/*
    Created-by: Floris Vrij (floows-s)
    Purpose: A class that makes it easy to connect to WiFi using the ESP-IDF.
*/

#pragma once
#include <cstring> 

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "WifiStationConfig.hpp"
#include "wifi_station_status.hpp"

// --- DEFINES ---
#define WIFI_EVENT_GRP_BIT_FAILED_TO_CONNECT (1 << 0)
#define WIFI_EVENT_GRP_BIT_CONNECTED (1 << 1)
#define WIFI_EVENT_GRP_BIT_DISCONNECTED (1 << 2)
#define WIFI_EVENT_GRP_BIT_RECONNECTING (1 << 3)

class WifiStation{
public: 
    WifiStation(WifiStationConfig config);
    ~WifiStation();

    /* GETTERS AND SETTERS */
    /// @brief Returns true if the initialize method has succefully executed.
    bool has_initialized_connection(){ return m_is_initialized; }
    const WifiStationConfig get_config(){ return m_config; }
    /// @brief Current status.
    wifi_station_status get_status() { return m_status; }

    /* METHODS */
    /// @brief Initalizes all necesary things for making a wifi connection.
    ///        Important: esp_netif_init() & esp_event_loop_create_default() should be called before calling this function.
    esp_err_t initialize();

    /// @brief Starts the wifi driver and try's to connect to the wifi AP.  
    ///        Important: Call initialize() before calling this method. 
    /// @return If connected succesfully.
    bool start_connection();
private: 
    /* METHODS */
    /// @brief Logs the SSID, channel, auth mode and AID.
    /// @param data 
    void log_wifi_information_from_connected_event(wifi_event_sta_connected_t* data);

    /// @brief Attempts to make wifi connection.
    /// @return - ESP_OK: succeed 
    ///         - ESP_ERR_WIFI_NOT_INIT: WiFi is not initialized (Forgot to call initialize()?)
    ///         - ESP_ERR_WIFI_NOT_STARTED: WiFi is not started by esp_wifi_start (Forgot to call start_connection()?) (Note: start_connection() calls this method) 
    ///         - ESP_ERR_WIFI_MODE: WiFi mode error 
    ///         - ESP_ERR_WIFI_CONN: WiFi internal error, station or soft-AP control block wrong 
    ///         - ESP_ERR_WIFI_SSID: SSID of AP which station connects is invalid
    esp_err_t attempt_connection();

    // --- Event handlers ---
    static void wifi_event_handler(void *handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

    /* MEMBERS */
    const char* m_log_tag = "WifiStation";
    const WifiStationConfig m_config;
    wifi_station_status m_status = wifi_station_status::DISCONNECTED;

    int m_connection_attempts = 0;  
    bool m_is_initialized = false;
    
    wifi_config_t m_wifi_config;

    EventGroupHandle_t m_event_group;

    esp_event_handler_instance_t m_wifi_handler_event_instance;
    esp_event_handler_instance_t m_got_ip_event_instance;
};