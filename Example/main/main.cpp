#include <stdio.h>

#include <iostream>
#include <string>

#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "WifiStation.hpp"

WifiStationConfig config {
    .ssid = "EXAMPLE_SSID",
    .password = "EXAMPLE_PASSWORD"
};

WifiStation station(config);

extern "C" void app_main(void)
{
  const char* log_tag = "app_main";

  // Initialize storage
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // Initialize the esp network interface
  ESP_ERROR_CHECK(esp_netif_init());

  // Initialize default esp event loop
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  // Initialize everything needed to make a wifi connection
  esp_err_t init_ret = station.initialize();

  if(init_ret != ESP_OK){
    ESP_LOGE(log_tag, "Error(%s): Failed to initialize WifiStation", esp_err_to_name(init_ret));
    return;
  }

  // Start wifi connection
  bool connected = station.start_connection();
  if(!connected){
    ESP_LOGE(log_tag, "Can't connect to wifi...");
    return;
  }
}
