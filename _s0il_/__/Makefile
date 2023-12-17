all: httpd httpd.xtensa
clean:
	rm -f httpd *.o httpd.xtensa

CC_NATIVE=@   echo 'host        CC ' $@ ; $(CC)
CC_DEVICE=@   echo 'xtensa      CC ' $@ ; xtensa-esp32s3-elf-gcc
POST_DEVICE=@ echo 'xtensa   strip ' $@ ; xtensa-esp32s3-elf-strip 
POST_NATIVE=@ echo 'host strip_pie ' $@ ; ../elf_strip_pie

CFLAGS=-Wall -Wextra -Wno-unused-parameter -g
CFLAGS+= -I. -I.. -I../..
CFLAGS+= -fsingle-precision-constant -Wdouble-promotion

CFLAGS_NATIVE = $(CFLAGS) `pkg-config ctx --cflags --libs` \
                   -g -lm -DNATIVE -fPIC -rdynamic -fpic \
                   -Wl,-E,--unresolved-symbols=ignore-all

CFLAGS_DEVICE = $(CFLAGS) -nostartfiles -nostdlib -fPIC -shared -e main \
  -fdata-sections -ffunction-sections -Wl,--gc-sections -fvisibility=hidden \
  -Wno-sign-compare \
  -DCTX_FLOW3R -DCTX_ESP \
  \
  -I../flow3r/build/config \
  -I$(IDF_PATH)/components/lwip/include \
  -I$(IDF_PATH)/components/lwip/lwip/src/include \
  -I$(IDF_PATH)/components/lwip/port/include \
  -I$(IDF_PATH)/components/lwip/port/freertos/include \
  -I$(IDF_PATH)/components/lwip/port/esp32xx/include \
  -I$(IDF_PATH)/components/newlib/platform_include \
  -I$(IDF_PATH)/components/esp_system/include \
  -I$(IDF_PATH)/components/esp_common/include \
  -I$(IDF_PATH)/components/heap/include \
  -I$(IDF_PATH)/components/esp_rom/include \
  -I$(IDF_PATH)/components/esp_timer/include \
  -I$(IDF_PATH)/components/xtensa/include \
  -I$(IDF_PATH)/components/esp_hw_support/include \
  -I$(IDF_PATH)/components/soc/esp32s3/include \
  -I$(IDF_PATH)/components/xtensa/esp32s3/include \
  -I$(IDF_PATH)/components/freertos/esp_additions/include/ \
  -I$(IDF_PATH)/components/freertos/esp_additions/include/freertos \
  -I$(IDF_PATH)/components/freertos/esp_additions/arch/xtensa/include \
  -I$(IDF_PATH)/components/freertos/FreeRTOS-Kernel/portable/xtensa/include/ \
  -I$(IDF_PATH)/components/freertos/FreeRTOS-Kernel/include 

%.o: %.c *.h Makefile
	$(CC_NATIVE) $(CFLAGS_NATIVE) -c $< -o $@
httpd: httpd.o
	$(CC_NATIVE) $(CFLAGS_NATIVE) $^ ../libui.so `pkg-config --cflags --libs ctx` -lm -o $@ 
	$(POST_NATIVE) $@

httpd.xtensa: *.c Makefile
	$(CC_DEVICE) $(CFLAGS_DEVICE) *.c -o $@
	$(POST_DEVICE) $@
