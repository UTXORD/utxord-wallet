export const PROD_URL = 'https://api.utxord.com';
export const STAGE_URL = 'https://api.qa.utxord.com';
export const TEMP_STAGE_URL = 'http://10.1.10.100:8001';
export const ETWOE_URL ='https://e2e.utxord.com';
export const TESTNET = 't';
export const MAINNET = 'm';
export const REGTEST = 'r';
export const SIGNET = 's';

export const BACKEND_URL = TEMP_STAGE_URL;

// NOTE: Configure it before build
export const NETWORK = TESTNET;

export const PROD_URL_PATTERN = 'https://utxord.com/*';
export const STAGE_URL_PATTERN = 'https://qa.utxord.com/*';
export const TEMP_STAGE_URL_PATTERN = 'http://10.1.10.100:8000/*';
export const ETWOE_URL_PATTERN = 'https://e2e.utxord.com/*';
export const LOCAL_URL_PATTERN = 'http://localhost:9000/*';

// NOTE: Configure it before build
export const BASE_URL_PATTERN = TEMP_STAGE_URL_PATTERN;
