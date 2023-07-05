.PHONY: clean all core-clean core ext-clean ext-dev ext-qa ext-e2e ext-utxord ext

ROOT_DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

CORE_DIR := ./core
CORE_BUILD_DIR := $(CORE_DIR)/bild

EXT_DIR := ./browser-extension
EXT_LIB_DIR := $(EXT_DIR)/src/libs
EXT_BUILD_DIR := $(EXT_DIR)/extension

CORE_TARGET := $(EXT_LIB_DIR)/utxord

core: $(CORE_TARGET).wasm
core-clean:
	rm -rf $(CORE_BUILD_DIR)

$(CORE_TARGET).wasm: $(shell find $(CORE_DIR) -type f)
	(cd $(CORE_DIR) ; ./autogen.sh)
	mkdir -p $(CORE_BUILD_DIR)
	(cd $(CORE_BUILD_DIR) ; emconfigure ../configure --enable-wasm-binding)
	(cd $(CORE_BUILD_DIR) ; emmake make -j2)
	cp -f $(CORE_BUILD_DIR)/src/wasm_binding/utxord.wasm $(CORE_TARGET).wasm
	cp -f $(CORE_BUILD_DIR)/src/wasm_binding/utxord.js $(CORE_TARGET).js

ext-clean:
	rm -rf $(EXT_BUILD_DIR)/*

ext-dev: core
	(cd $(EXT_DIR) ; yarn install && yarn dev)

ext-qa: core
	(cd $(EXT_DIR) ; yarn install && yarn build-qa)

ext-e2e:
	(cd $(EXT_DIR) ; yarn install && yarn build-e2e)

ext-utxord:
	(cd $(EXT_DIR) ; yarn install && yarn build-utxord)

ext: ext-e2e

clean: core-clean ext-clean

all: ext

