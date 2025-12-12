/*
    Created-by: Floris Vrij (floows-s)
    Purpose: Source code for the WifiStation class. 
             A class that makes it easy to connect to WiFi using the ESP-IDF.
*/

#pragma once
#include "WifiStation.hpp"

#include <cstring> 

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

WifiStation::WifiStation(WifiStationConfig config) : m_config(config){}

WifiStation::~WifiStation(){
    if(m_is_initialized){
        // Unregister event handlers
        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(
            IP_EVENT, 
            IP_EVENT_STA_GOT_IP, 
            m_got_ip_event_instance
        ));

        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(
            WIFI_EVENT, 
            ESP_EVENT_ANY_ID, 
            m_wifi_handler_event_instance
        ));

        // Delete event group
        vEventGroupDelete(m_event_group);
    }
}

// Initalizes the wifi driver, registers handlers and creates wifi event group. 
esp_err_t WifiStation::initialize(){ 
    if(m_is_initialized){
        return ESP_OK;
    }

    ESP_LOGI(m_log_tag, "Initializing...");

    /* WIFI DRIVER */
    // Create wifi station in the network interface
    esp_netif_create_default_wifi_sta();

    // Init wifi drivers with default wifi configuration
    wifi_init_config_t def_config = WIFI_INIT_CONFIG_DEFAULT(); 
    ESP_ERROR_CHECK(esp_wifi_init(&def_config));

    // Set the wifi mode to station
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); 

    // Create wifi config
    m_wifi_config = {
        .sta = {
            .threshold = {
                .authmode = WIFI_AUTH_WPA2_PSK, // Authentication mode
            },
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        }
    };


    unsigned int null_termintor_size = sizeof('\n') / sizeof(char);

    // Set SSID
    // Prevent overflow
    unsigned int max_ssid_length = sizeof(m_wifi_config.sta.ssid) / sizeof(m_wifi_config.sta.ssid[0]);
    if((m_config.ssid.length() + null_termintor_size) > max_ssid_length){
        ESP_LOGE(m_log_tag, "Error: Given SSID is to long. Max length is %d characters.", max_ssid_length - null_termintor_size);
        ESP_LOGE(m_log_tag, "Given SSID: %s", m_config.ssid.c_str());
        return ESP_ERR_INVALID_SIZE;
    }

    strcpy((char*)m_wifi_config.sta.ssid, m_config.ssid.c_str());



    // Set password
    // Prevent overflow
    unsigned int max_password_length = sizeof(m_wifi_config.sta.password) / sizeof(m_wifi_config.sta.password[0]);
    if((m_config.password.length() + null_termintor_size) > max_password_length){
        ESP_LOGE(m_log_tag, "Error: Given password is to long. Max length is %d characters.", max_password_length - null_termintor_size);
        ESP_LOGE(m_log_tag, "Given password: %s", m_config.password.c_str());
        return ESP_ERR_INVALID_SIZE;
    }

    strcpy((char*)m_wifi_config.sta.password, m_config.password.c_str());

    
    // Set the wifi config (with ssid and password)
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &m_wifi_config));

    /* REGISTER HANDLERS */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, // The event base -> every wifi event
        ESP_EVENT_ANY_ID, // Any type of event (from the WIFI_EVENT)
        &wifi_event_handler, // The handler
        this, // Argument passed to the handler
        &m_wifi_handler_event_instance // Save the event handler registration to this property so it can be removed later on
    )); 

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, // The event base -> every ip event
        IP_EVENT_STA_GOT_IP, // Only station got ip event -> if the esp has received a ip-adress from the router
        &ip_event_handler, // The handler
        this, // Arguments oassed to the handler
        &m_got_ip_event_instance // Save the event handler registration to this property so it can be removed later on
    ));

    /* EVENT GROUP */
    m_event_group = xEventGroupCreate();

    ESP_LOGI(m_log_tag, "Intialisasion completed!");
    m_is_initialized = true;

    return ESP_OK;
}

bool WifiStation::start_connection(){
    if(!m_is_initialized){
        initialize();
    }

    // Start wifi driver
    m_status = wifi_station_status::STARTING_CONNECTION;
    ESP_ERROR_CHECK(esp_wifi_start()); 
    ESP_LOGI(m_log_tag, "Starting wifi driver...");

    // Note: Beyond this point the registerd handlers can be triggerd...

    // This will block the code until the wifi has either connected or failed to connect
    EventBits_t bits = xEventGroupWaitBits(
        m_event_group,
        WIFI_EVENT_GRP_BIT_CONNECTED | WIFI_EVENT_GRP_BIT_FAILED_TO_CONNECT, // Bits to wait for, the | operator combines the two binary values
        pdFALSE, // Clear bit in group on exit -> False
        pdFALSE, // Wait for all specified bits -> False
        portMAX_DELAY // Time to wait for bits -> Indefinitely
    ); 

    if (bits & WIFI_EVENT_GRP_BIT_FAILED_TO_CONNECT) {
        ESP_LOGI(m_log_tag, "Failed to connect to wifi.");
        return false;
    }

    // Note: m_status == wifi_station_status::CONNECTED is set in wifi_event_handler() -> WIFI_EVENT_STA_CONNECTED

    return true;
}

esp_err_t WifiStation::attempt_connection(){
    m_connection_attempts++;
    ESP_LOGI(m_log_tag, "Connection attempt: %d", m_connection_attempts);

    esp_err_t result = esp_wifi_connect();
    if(result != ESP_OK){
        // Dont abort with ESP_ERROR_CHECK()
        // Just log the error and continue
        ESP_LOGW(m_log_tag, "Error (%d) while connecting to wifi", result);
    }

    return result;
}

void WifiStation::log_wifi_information_from_connected_event(wifi_event_sta_connected_t* event_sta_connected){
    ESP_LOGI(m_log_tag, "----- CONNECTION INFORMATION -----");
    ESP_LOGI(m_log_tag, "SSID: %s", (char *)event_sta_connected->ssid);
    ESP_LOGI(m_log_tag, "Channel: %d", event_sta_connected->channel);
    ESP_LOGI(m_log_tag, "Auth mode: %d", event_sta_connected->authmode);
    ESP_LOGI(m_log_tag, "AID: %d", event_sta_connected->aid);
    ESP_LOGI(m_log_tag, "----------------------------------");
}


/* EVENT HANDLERS */

void WifiStation::wifi_event_handler(void* handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    WifiStation* self = static_cast<WifiStation*>(handler_arg);

    if(event_base != WIFI_EVENT) {
        return;
    };

    switch(event_id){
        case WIFI_EVENT_STA_START: // The esp wifi station backend has just intialized
            // Connect to wifi
            ESP_LOGI(self->m_log_tag, "Connecting to wifi...");
            self->attempt_connection();
            break;

        case WIFI_EVENT_STA_CONNECTED: // Wifi is connected
            ESP_LOGI(self->m_log_tag, "Connected to wifi!");
            self->log_wifi_information_from_connected_event(
                static_cast<wifi_event_sta_connected_t*>(event_data)
            );

            self->m_connection_attempts = 0; // Reset counter
            self->m_status = wifi_station_status::CONNECTED;
            break;

        case WIFI_EVENT_STA_DISCONNECTED: // Wifi has disconnected (This event will also be triggerd when a connection attempt has failed)
            if(self->m_status == wifi_station_status::STARTING_DISCONNECTION){
                self->m_status = wifi_station_status::DISCONNECTED;
                return;
            }

            /* RECONNECT */
            // Note: We want the same "auto-reconnect" behavioure when starting a connection.
            //       This is so it can have some attempts at connecting.
            if(self->m_config.auto_reconnect || self->m_status == wifi_station_status::STARTING_CONNECTION){
                if(self->m_connection_attempts > self->m_config.max_con_failures){
                    ESP_LOGW(self->m_log_tag, "Couldnt connect to wifi: Max connection failures reached");

                    self->m_status = wifi_station_status::DISCONNECTED;
                    xEventGroupSetBits(self->m_event_group, WIFI_EVENT_GRP_BIT_FAILED_TO_CONNECT);
                    return;
                }

                if(self->m_config.max_con_failures < 0 && self->m_connection_attempts > 20){
                    // Here i could put it into deep sleep and try to reconnect every 10 minutes to save power
                }

                if(self->m_status != wifi_station_status::STARTING_CONNECTION){
                    self->m_status = wifi_station_status::RECONNECTING;

                    if(self->m_connection_attempts == 0){
                        // Only print on first reconnection attempt
                        ESP_LOGI(self->m_log_tag, "Reconnecting to wifi...");
                    }
                }

                if(self->m_connection_attempts > 0){
                    // Put a small delay so it doesn't spam reconnect
                    vTaskDelay(500 / portTICK_PERIOD_MS);
                }

                // Try to reconnect
                self->attempt_connection();
            }
            break;
    }
}

void WifiStation::ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    WifiStation* self = static_cast<WifiStation*>(arg);

    if(event_base != IP_EVENT){
        return;
    }

    if (event_id == IP_EVENT_STA_GOT_IP){ // IP_EVENT_STA_GOT_IP -> when the router has given the station (esp32) an ip adress
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        
        ESP_LOGI(self->m_log_tag, "Gotten a IP: " IPSTR, IP2STR(&event->ip_info.ip));

        xEventGroupSetBits(self->m_event_group, WIFI_EVENT_GRP_BIT_CONNECTED);
        self->m_status = wifi_station_status::CONNECTED;
    }
}
