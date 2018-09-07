all: test_httpc

OBJS := main.o ../httpc.o $(IDF_PATH)/components/esp-tls/esp_tls.o ../../http-parser/http-parser/http_parser.o 
CFLAGS := -I. -I.. -I$(IDF_PATH)/components/esp-tls -I../../http-parser/http-parser $(EXTRA_CFLAGS) -g

test_httpc: $(OBJS)
	gcc -g -o $@ $(OBJS) -lmbedtls -lmbedcrypto -lmbedx509 $(EXTRA_LDFLAGS)

clean:
	rm -f test_httpc
