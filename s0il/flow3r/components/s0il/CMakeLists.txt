idf_component_register(
   SRCS
      s0il.c
      s0il-main.c
      bundled-programs.c
   INCLUDE_DIRS
      "." ".." "../../../../" "../../../" "../../../../deps" "../../../../fonts"
   REQUIRES
      driver
      esp-tls
      elf_loader
      esp_http_client
      vfs
)

