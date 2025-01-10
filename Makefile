.PHONY: clean all core-clean core ext-clean ext-build-clean ext-core-lib ext-deps ext-dev ext-qa-firefox ext-e2e-firefox ext-utxord-firefox ext-qa ext-e2e ext-utxord ext

ROOT_DIR = $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

CORE_DIR = ./core
CORE_BUILD_DIR = $(CORE_DIR)/build_wasm
CORE_BUILD_CONFIG = $(CORE_DIR)/configure
CORE_BUILD_MAKEFILE = $(CORE_BUILD_DIR)/Makefile
CORE_TARGET_DIR = $(CORE_BUILD_DIR)/src/wasm_binding
CORE_TARGETS = $(CORE_TARGET_DIR)/utxord.wasm $(CORE_TARGET_DIR)/utxord.js

EXT_DIR = ./browser-extension
EXT_LIB_DIR = $(EXT_DIR)/src/libs
EXT_BUILD_DIR = $(EXT_DIR)/extension

EXT_MODULE_FILES = $(EXT_DIR)/node_modules $(EXT_DIR)/yarn.lock $(EXT_DIR)/package-lock.json
EXT_LIB_FILES = $(EXT_LIB_DIR)/utxord.wasm  $(EXT_LIB_DIR)/utxord.js

$(CORE_BUILD_CONFIG): $(CORE_DIR)/configure.ac
	(cd $(CORE_DIR) ; ./autogen.sh)

$(CORE_BUILD_MAKEFILE): $(CORE_BUILD_CONFIG)
	mkdir -p $(CORE_BUILD_DIR)
	(cd $(CORE_BUILD_DIR) ; emconfigure ../configure --enable-wasm-binding)

$(CORE_TARGETS): $(CORE_BUILD_MAKEFILE) $(shell find $(CORE_DIR) -not \( -path $(CORE_BUILD_DIR) -prune \) -type f)
	(cd $(CORE_BUILD_DIR) ; emmake $(MAKE))

core: $(CORE_TARGETS)

core-clean:
	rm -rf $(CORE_BUILD_DIR)

ext-build-clean:
	rm -rf $(EXT_BUILD_DIR)/*

ext-clean: ext-build-clean
	rm -rf $(EXT_LIB_FILES) $(EXT_MODULE_FILES)

clean: core-clean ext-clean

ext-core-lib: $(CORE_TARGETS)
	cp -f $(CORE_TARGET_DIR)/utxord.wasm $(EXT_LIB_DIR)
	sed 's/^var utxord = /self.utxord = /' $(CORE_TARGET_DIR)/utxord.js  > $(EXT_LIB_DIR)/utxord.js

ext-test-core-lib: $(CORE_TARGETS)
	cp -f $(CORE_TARGET_DIR)/utxord.wasm $(EXT_LIB_DIR)
	cp -f $(CORE_TARGET_DIR)/utxord.js $(EXT_LIB_DIR)
	echo "export { utxord as utxord };" >> $(EXT_LIB_DIR)/utxord.js

ext-deps: ext-core-lib
	(cd $(EXT_DIR) ; npm install)

ext-test-deps: ext-test-core-lib
	(cd $(EXT_DIR) ; npm install)

ext-dev: ext-deps
	(cd $(EXT_DIR) ; npm run dev)

ext-qa-firefox: ext-deps
	(cd $(EXT_DIR) ; npm run build-qa-firefox)

ext-e2e-firefox: ext-deps
	(cd $(EXT_DIR) ; npm run build-e2e-firefox)

ext-utxord-firefox: ext-deps
	(cd $(EXT_DIR) ; npm run build-utxord-firefox)

ext-qa: ext-deps
	(cd $(EXT_DIR) ; npm run build-qa)

ext-e2e: ext-deps
	(cd $(EXT_DIR) ; npm run build-e2e)

ext-utxord: ext-deps
	(cd $(EXT_DIR) ; npm run build-utxord)

ext-test: ext-test-deps
	(cd $(EXT_DIR) ; npm run test)

ext: ext-utxord

all: ext
