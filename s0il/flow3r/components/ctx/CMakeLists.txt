idf_component_register(
   SRCS
      ctx.c
      gc9a01.c
   INCLUDE_DIRS
      "." ".." "../.." "../../../../" "../../../" "../../../../deps" "../../../../fonts"
   REQUIRES
      driver
      vfs
      fatfs
      esp_event
      esp_wifi
      flow3r_bsp
      nvs_flash
)

