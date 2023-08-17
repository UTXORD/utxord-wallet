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

## How to install it from this repository

1) Download latest *utxord-wallet-v1.0.x.zip*
 file from https://github.com/UTXORD/utxord-wallet/releases
2) Unpack it to some folder.
3) In Chrome press **...** icon on top-right to open a menu.
4) Select items **Extensions** -> **Manage Extensions**.
5) Turn on **Developer mode** slider on top-right edge of **Extensions** screen.
6) Click **Load unpacked** button at top-left edge of  **Extensions** screen then select unpacked plugin folder
   and press **Select** button.
7) Now you should be able to see **UTXORD Wallet** extension as installed and active.
8) Press **Extensions** icon on Chrome toolbar.
9) Click on **Pin** icon at **UTXORD Wallet** item to pin its icon on a toolbar.
