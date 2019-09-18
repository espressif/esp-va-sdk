# We also need to manually add `LOGI` to esp-tls.c

all: test_httpc

OBJS := main.o ../httpc.o $(IDF_PATH)/components/esp-tls/esp_tls.o $(IDF_PATH)/components/nghttp/port/http_parser.o
CFLAGS := -I. -I.. -I$(IDF_PATH)/components/esp-tls -I$(IDF_PATH)/components/nghttp/port/include/ $(EXTRA_CFLAGS) -g

test_httpc: $(OBJS)
	gcc -g -o $@ $(OBJS) -lmbedtls -lmbedcrypto -lmbedx509 $(EXTRA_LDFLAGS)

clean:
	rm -f test_httpc
