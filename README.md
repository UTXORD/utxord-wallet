# UTXORD Wallet

UTXORD Wallet Chrome extension consists of core WASM module and extension itself. It uses a number submodules, so you
have to  clone this repository using ```git clone --recurse-submodules``` command. In case you have already cloned this
repository without submodules you need to run ```git submodule update --init``` command.

## Build sequence
1) *docker compose build*
2) *docker compose run toolchain-shell make clean \<ext-target\>*

Extension targets:
- *ext*, *ext-e2e* - extension for end-to-end testing environment
- *ext-qa* - extension for QA environment
- *ext-utxord* - extension for production environment
