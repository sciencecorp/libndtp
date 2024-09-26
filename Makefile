PROJECT_NAME := synapsectl
PROTOC := protoc
PROTO_DIR := ./synapse-api
PROTO_OUT := ./synapse
PROTOS := $(shell find ${PROTO_DIR} -name '*.proto' | sed 's|${PROTO_DIR}/||')

CXX := g++
PROTOC := protoc
CXXFLAGS := -std=c++17 -I. -I${PROTO_OUT}
LDFLAGS := -L${PROTO_OUT} -lprotobuf

CPP_FILES := $(PROTOS:.proto=.pb.cc)
H_FILES := $(PROTOS:.proto=.pb.h)
GRPC_CPP_FILES := $(PROTOS:.proto=.grpc.pb.cc)
GRPC_H_FILES := $(PROTOS:.proto=.grpc.pb.h)

.PHONY: all
all: generate

.PHONY: clean
clean:
	@echo "Cleaning generated files..."
	@rm -rf ${PROTO_OUT}

.PHONY: generate
generate: 
	@echo "Generating C++ files from Protocol Buffers..."
	@mkdir -p ${PROTO_OUT}
	@for proto in ${PROTOS}; do \
		echo "Processing $$proto"; \
		${PROTOC} -I=${PROTO_DIR} --cpp_out=${PROTO_OUT} ${PROTO_DIR}/$$proto; \
	done

${PROTO_OUT}:
	mkdir -p ${PROTO_OUT}

${PROTO_OUT}/%.pb.cc ${PROTO_OUT}/%.pb.h: ${PROTO_DIR}/%.proto
	${PROTOC} -I=${PROTO_DIR} --cpp_out=${PROTO_OUT} $<

.PHONY: compile
compile: generate
	@echo "Compiling generated C++ files..."
	@${CXX} ${CXXFLAGS} -c $(addprefix ${PROTO_OUT}/,${CPP_FILES}) -o ${PROTO_OUT}/synapse_api.o

.PHONY: shared
shared: compile
	@echo "Creating shared library..."
	@${CXX} -shared -o ${PROTO_OUT}/libsynapse_api.so ${PROTO_OUT}/synapse_api.o ${LDFLAGS}

.PHONY: test
test:
	@echo "Running tests..."
	@pytest -v

.PHONY: info
info:
	@echo "Protocol Buffers found:"
	@echo ${PROTOS}