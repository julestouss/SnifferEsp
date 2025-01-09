#include <stdio.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_check.h>
#include <nvs_flash.h> // to flash the memory (Non volatile sorage = NVS)
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"


#define WIFI_FAILURE 1<<1
#define WIFI_SUCCESS 1<<0

#define TAG "Wifi :"

#define ESP_WIFI_SSID "test_esp"
#define ESP_WIFI_PASS "motdepass"
#define ESP_WIFI_CHANNEL 1
#define ESP_WIFI_MAX_CONN 10


static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){

    // wifi station start event 
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START){
        ESP_LOGI(TAG, "First Connection ....");
        esp_wifi_connect();
        // event trigerred when the wifi sta start
        // esp_wifi_connect is use to initialise the wifi connexion
    
    // wifi station disconnect event 
        if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){
        int i = 0;
            while (i < 10 || event_id == WIFI_EVENT_STA_DISCONNECTED){
                esp_wifi_connect();
                ESP_LOGI(TAG, "connecting to AP .....");
                vTaskDelay(500);
                i++;
                if(i == 10){
                    ESP_LOGI(TAG, "failed to connect");
                    break; 
                }
            
                // trigerred when the esp is disconnected from the esp
                // here esp_wifi_connect is used to reconnect and maintain a stable connexion
                // we try to reconnect 10 times to avoid micro wifi disconnect
            }
        }
    }
    if (event_base == WIFI_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "Connection Succesful");
    
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI("WIFI_EVENT", "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        // here we get the ip and store in event
    }
    if( event_base == WIFI_EVENT && event_id == ESP_ERR_WIFI_PASSWORD ) printf("Bad Password");

    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station joined");
    } // here we handle when the AP is up and then we print the mac address in the log 
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
       wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
       ESP_LOGI(TAG, "station leaved"); //here we print in the log if the ap is disconnected

   }
}

esp_err_t conf_wifi (){
    // initialise the networking interface
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default()); // create the event that will be used by wifi to handle the events
    // create wifi sta in the wifi drivre
    esp_netif_create_default_wifi_sta();
    //create wifi ap in the wifi drive
    esp_netif_create_default_wifi_ap();
    // define the wifi conf with default conf 
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    //initialise the wifi module with the conf that we defined before
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    

    

        // event handler register, it save it in the event handler from esp-idf and link them to the spesified callback func
        esp_event_handler_instance_t wifi_handler_event_instance;
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &event_handler,
                                                            NULL,
                                                            &wifi_handler_event_instance)); // save every WIFI_EVENT in event_handler, event_handler will be called for each wifi event
                                                                                           
        esp_event_handler_instance_t got_ip_event_instance;
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                            IP_EVENT_STA_GOT_IP,
                                                            &event_handler,
                                                            NULL,
                                                            &got_ip_event_instance));// save every IP_EVENT in event handler, call for ip event 


    // define and set the wifi conf

    wifi_config_t sta_wifi_config = {
        // use to connect to a station
        .sta ={
            .ssid = "Altice_F524",
            .password = "tscp1065",
	        .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
                },
        },
    };
    //use to setup the AP
    wifi_config_t ap_wifi_config = {   
        .ap = {
                .ssid = ESP_WIFI_SSID,
                .ssid_len = strlen(ESP_WIFI_SSID),
                .password = ESP_WIFI_PASS,
                .channel = ESP_WIFI_CHANNEL,
                .max_connection = ESP_WIFI_MAX_CONN,
                .authmode = WIFI_AUTH_WPA2_PSK,
            },

    };


    //define the wifi mod
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    // set sta conf
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_wifi_config));

    // set ap conf 
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_wifi_config));

    // start wifi
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "wifi_init_sta finished.");
    
    return WIFI_SUCCESS;
}


void app_main(void) {
    // initialise nvs 
    esp_err_t ret = nvs_flash_init(); // is use to check if an error occure and then init the nvs flash
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
            // if the nvs is not blank erase what is written in it 
            }
    ESP_ERROR_CHECK(ret);
    

    // check if something is going wronge while connecting to the ap
    esp_err_t wifi_status = WIFI_FAILURE;

    wifi_status = conf_wifi();
    if (WIFI_SUCCESS != wifi_status){
        ESP_LOGI("WIFI ", "Failed to connect to the sta, dying ....");
        return;

    }

}
