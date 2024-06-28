export const PROD_URL = 'https://api.utxord.com';
export const STAGE_URL = 'https://api.qa.utxord.com';
export const TEMP_STAGE_URL = 'http://10.1.10.100:8001';
export const ETWOE_URL ='http://api.e2e.utxord.com:8000';
export const TESTNET = 't';
export const MAINNET = 'm';
export const REGTEST = 'r';
export const SIGNET = 's';

export const BACKEND_URL = TEMP_STAGE_URL;

// NOTE: Configure it before build
export const NETWORK = SIGNET;

export const PROD_URL_PATTERN = 'https://utxord.com/*';
export const STAGE_URL_PATTERN = 'https://qa.utxord.com/*';
export const TEMP_STAGE_URL_PATTERN = 'http://10.1.10.100:8080/*';
export const ETWOE_URL_PATTERN = 'http://e2e.utxord.com:9000/*';
export const LOCAL_URL_PATTERN = 'http://localhost:9000/*';

// NOTE: Configure it before build
export const BASE_URL_PATTERN = STAGE_URL_PATTERN;
