# UTXORD Chrome extension


Extension consists of two parts:

- Core WASM module
- Extension itself

## Build sequence
1) *docker compose build*
2) *docker compose run toolchain-shell make clean <ext-target>*

Extension targets:
- *ext*, *ext-e2e* - extension for end-to-end testing environment
- *ext-qa* - extension for QA environment
- *ext-utxord* - extension for production environment
