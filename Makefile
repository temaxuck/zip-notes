CC       ?= cc
CC_FLAGS ?= -Wall -Wextra

BUILD_DIR  ?= build/
TARGET_DIR ?= foo/

DEBUG ?= 0
ifeq ($(DEBUG), 1)
	CC_FLAGS += -ggdb
endif

all: build archive

build: build_dir
	$(CC) $(CC_FLAGS) -o $(BUILD_DIR)main main.c

archive: build_dir
	zip -r $(BUILD_DIR)archive.zip $(TARGET_DIR)

build_dir:
	mkdir -p $(BUILD_DIR)
