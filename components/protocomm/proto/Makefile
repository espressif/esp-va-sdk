
all: pre_check c_proto python_proto

pre_check:
ifeq ($(PROTOC_C),)
	@echo "Specify PROTOC_C to point to protoc-c compiler on your system"
	exit 1
endif

c_proto: *.proto
	@$(PROTOC_C) --c_out=../proto-c/ -I . *.proto

python_proto: *.proto
	@protoc --python_out=../python/protocomm/ -I . *.proto
