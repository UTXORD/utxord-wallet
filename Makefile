.PHONY: clean all core-clean core ext-clean ext-dev ext-qa ext-e2e ext-utxord ext

ROOT_DIR = $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

CORE_DIR = ./core
CORE_BUILD_DIR = $(CORE_DIR)/build
CORE_TARGET_DIR = $(CORE_BUILD_DIR)/src/wasm_binding
CORE_TARGETS = $(CORE_TARGET_DIR)/utxord.wasm $(CORE_TARGET_DIR)/utxord.js

EXT_DIR = ./browser-extension
EXT_LIB_DIR = $(EXT_DIR)/src/libs
EXT_BUILD_DIR = $(EXT_DIR)/extension


core: $(CORE_TARGET_DIR)/utxord.wasm
core-clean:
	rm -rf $(CORE_BUILD_DIR)

$(CORE_TARGETS): $(shell find $(CORE_DIR) -not \( -path $(CORE_BUILD_DIR) -prune \) -type f)
	(cd $(CORE_DIR) ; pwd ; ls -al ; ./autogen.sh)
	mkdir -p $(CORE_BUILD_DIR)
	(cd $(CORE_BUILD_DIR) ; emconfigure ../configure --enable-wasm-binding)
	(cd $(CORE_BUILD_DIR) ; emmake make -j4)

ext-clean:
	rm -rf $(EXT_BUILD_DIR)/* $(EXT_LIB_DIR)/utxord.wasm  $(EXT_LIB_DIR)/utxord.js $(EXT_DIR)/node_modules

ext-core-lib: $(CORE_TARGETS)
	cp -f $(CORE_TARGET_DIR)/utxord.wasm $(EXT_LIB_DIR)
	sed 's/^var utxord = /self.utxord = /' $(CORE_TARGET_DIR)/utxord.js  > $(EXT_LIB_DIR)/utxord.js

ext-deps: ext-core-lib
	(cd $(EXT_DIR) ; yarn install)

ext-dev: ext-deps
	(cd $(EXT_DIR) ; yarn dev)

ext-qa: ext-deps
	(cd $(EXT_DIR) ; yarn build-qa)

ext-e2e: ext-deps
	(cd $(EXT_DIR) ; yarn build-e2e)

ext-utxord: ext-deps
	(cd $(EXT_DIR) ; yarn build-utxord)

ext: ext-qa

clean: core-clean ext-clean

all: ext

