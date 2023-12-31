
#CCACHE=`command -v ccache`  ## mostly pointless as we compile few .o files

CFLAGS+=-O2 -g -DS0IL -fsingle-precision-constant \
  -Wall -Wextra \
  -Wdouble-promotion -Wwrite-strings -Wchar-subscripts \
  \
  -Wno-format -Wno-sign-compare -Wno-unused-parameter \
  -Wno-deprecated-declarations -Wno-discarded-qualifiers -Wno-clobbered
CFLAGS+= -I. -I..

CC_NATIVE=@   echo 'CC     ' $@ ; $(CCACHE) $(CC)
CFLAGS_NATIVE = $(CFLAGS) -I.. -L.. -lctx `pkg-config libcurl --cflags --libs` \
                  -g -lm -DS0IL_NATIVE -fPIC -rdynamic -fpic \
                   -Wl,-E,--unresolved-symbols=ignore-all 

POST_NATIVE=@ echo 'rem-PIE' $@; ./elf_strip_pie $@
POST_NATIVE2=@ echo 'rem-PIE' $@; ../elf_strip_pie $@
POST_NATIVE3=@ echo 'rem-PIE' $@; ../../elf_strip_pie $@

CC_RISCV=@   echo 'CC     ' $@ ; $(CCACHE) riscv32-esp-elf-gcc

CC_XTENSA=@   echo 'CC     ' $@ ; $(CCACHE) xtensa-esp32s3-elf-gcc 
#CC_NATIVE=$(CC)
POST_XTENSA=@echo 'STRIP  ' $@ ; xtensa-esp32s3-elf-strip 
POST_RISCV=@echo 'STRIP  ' $@ ; riscv32-esp-elf-strip 
#POST_XTENSA=@ echo 'STRIP  ' $@ ; echo
#--remove-section=.xtensa.info --remove-section=.comment

CFLAGS_XTENSA = $(CFLAGS) -nostartfiles -nostdlib -fPIC -shared -e main \
  -fdata-sections -ffunction-sections -Wl,--gc-sections -fvisibility=hidden \
  -Wno-sign-compare \
  -DCTX_FLOW3R -DCTX_ESP \
  \
  -Iflow3r/build/config \
  -I../flow3r/build/config \
  -I../../flow3r/build/config \
  -I$(IDF_PATH)/components/lwip/include \
  -I$(IDF_PATH)/components/lwip/lwip/src/include \
  -I$(IDF_PATH)/components/lwip/port/include \
  -I$(IDF_PATH)/components/lwip/port/freertos/include \
  -I$(IDF_PATH)/components/newlib/platform_include \
  -I$(IDF_PATH)/components/heap/include \
  -I$(IDF_PATH)/components/esp_system/include \
  -I$(IDF_PATH)/components/esp_common/include \
  -I$(IDF_PATH)/components/esp_rom/include \
  -I$(IDF_PATH)/components/esp_timer/include \
  -I$(IDF_PATH)/components/esp_hw_support/include \
  -I$(IDF_PATH)/components/lwip/port/esp32xx/include \
  -I$(IDF_PATH)/components/xtensa/include \
  -I$(IDF_PATH)/components/soc/esp32s3/include \
  -I$(IDF_PATH)/components/xtensa/esp32s3/include \
  -I$(IDF_PATH)/components/freertos/esp_additions/include/ \
  -I$(IDF_PATH)/components/freertos/esp_additions/include/freertos \
  -I$(IDF_PATH)/components/freertos/esp_additions/arch/xtensa/include \
  -I$(IDF_PATH)/components/freertos/FreeRTOS-Kernel/portable/xtensa/include/ \
  -I$(IDF_PATH)/components/freertos/FreeRTOS-Kernel/include \
  -I$(IDF_PATH)/components/mbedtls/mbedtls/include

CFLAGS_RISCV = $(CFLAGS) -nostartfiles -nostdlib -fPIC -shared -e main -DRISCV \
  -fdata-sections -ffunction-sections -Wl,--gc-sections -fvisibility=hidden \
  -Wno-sign-compare \
  -DCTX_ESP \
  \
  -Iesp32c3/build/config \
  -I../esp32c3/build/config \
  -I../../esp32c3/build/config \
  -I$(IDF_PATH)/components/lwip/include \
  -I$(IDF_PATH)/components/lwip/lwip/src/include \
  -I$(IDF_PATH)/components/lwip/port/include \
  -I$(IDF_PATH)/components/lwip/port/freertos/include \
  -I$(IDF_PATH)/components/newlib/platform_include \
  -I$(IDF_PATH)/components/heap/include \
  -I$(IDF_PATH)/components/esp_system/include \
  -I$(IDF_PATH)/components/esp_common/include \
  -I$(IDF_PATH)/components/esp_rom/include \
  -I$(IDF_PATH)/components/esp_timer/include \
  -I$(IDF_PATH)/components/esp_hw_support/include \
  -I$(IDF_PATH)/components/lwip/port/esp32xx/include \
  -I$(IDF_PATH)/components/soc/esp32c3/include \
  -I$(IDF_PATH)/components/xtensa/riscv/include \
  -I$(IDF_PATH)/components/freertos/esp_additions/include/ \
  -I$(IDF_PATH)/components/freertos/esp_additions/include/freertos \
  -I$(IDF_PATH)/components/freertos/esp_additions/arch/riscv/include \
  -I$(IDF_PATH)/components/freertos/FreeRTOS-Kernel/portable/riscv/include/ \
  -I$(IDF_PATH)/components/freertos/FreeRTOS-Kernel/include \
  -I$(IDF_PATH)/components/riscv/include \
  -I$(IDF_PATH)/components/mbedtls/mbedtls/include

