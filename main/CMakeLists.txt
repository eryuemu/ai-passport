# main/CMakeLists.txt —— feature/world-execute-me 分支构建配置
idf_component_register(
    SRCS "main.c"
         "world_execute_player.c"
         "demo_world_execute.c"
         "tinf/tinflate.c"
         "tinf/tinfzlib.c"
         "tinf/adler32.c"
         "demo_navigation.c"
         "ui_pixel.c"
         "ui_pixel_math.c"
         "demo_display.c"
         "demo_button.c"
         "demo_audio.c"
         "demo_battery.c"
         "demo_radio.c"
         "demo_wifi.c"
         "demo_ble.c"
         "demo_low_power.c"
    INCLUDE_DIRS "." "tinf"
    REQUIRES bsp bt esp_event esp_hw_support esp_netif esp_timer esp_wifi nvs_flash
    EMBED_FILES "world_execute_data.bin"
)
