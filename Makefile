.PHONY: clean all core-clean core ext-clean ext-dev ext-qa ext-e2e ext-utxord ext

ROOT_DIR = $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

CORE_DIR = ./core
CORE_BUILD_DIR = $(CORE_DIR)/build
CORE_TARGET_DIR = $(CORE_BUILD_DIR)/src/wasm_binding
CORE_TARGETS = $(CORE_TARGET_DIR)/utxord.wasm $(CORE_TARGET_DIR)/utxord.js

EXT_DIR = ./browser-extension
EXT_LIB_DIR = $(EXT_DIR)/src/libs
EXT_BUILD_DIR = $(EXT_DIR)/extension

EXT_MODULE_FILES = $(EXT_DIR)/node_modules $(EXT_DIR)/yarn.lock $(EXT_DIR)/package-lock.json
EXT_LIB_FILES = $(EXT_LIB_DIR)/utxord.wasm  $(EXT_LIB_DIR)/utxord.js


$(CORE_TARGETS): $(shell find $(CORE_DIR) -not \( -path $(CORE_BUILD_DIR) -prune \) -type f)
	(cd $(CORE_DIR) ; ./autogen.sh)
	mkdir -p $(CORE_BUILD_DIR)
	(cd $(CORE_BUILD_DIR) ; emconfigure ../configure --enable-wasm-binding)
	(cd $(CORE_BUILD_DIR) ; emmake make -j4)

core: $(CORE_TARGETS)

core-clean:
	rm -rf $(CORE_BUILD_DIR)

ext-clean:
	rm -rf $(EXT_BUILD_DIR)/* $(EXT_LIB_FILES) $(EXT_MODULE_FILES)

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

ext: ext-utxord

clean: core-clean ext-clean

all: ext

