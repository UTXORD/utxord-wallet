from libutxord_pybind import *

print("Exceptions test")


#p2tr
try:
    p2tr_contract = CreateInscriptionBuilder(MAINNET, INSCRIPTION)
    p2tr_contract.OrdOutput(330, "bc1pp5t2a3j6fl8v7785szxeyhk8dpqksas7w5ta9j8caysn5ud8l68qcey6ak")
    p2tr_contract.AuthorFee(330, "bc1pp5t2a3j6fl8v7785szxeyhk8dpqksas7w5ta9j8caysn5ud8l68qcey6ak")
    p2tr_contract.MarketFee(330, "bc1pp5t2a3j6fl8v7785szxeyhk8dpqksas7w5ta9j8caysn5ud8l68qcey6ak")
    p2tr_contract.AddCustomFee(330, "bc1pp5t2a3j6fl8v7785szxeyhk8dpqksas7w5ta9j8caysn5ud8l68qcey6ak")

    p2tr_contract.AddUTXO("", 0, 330, "bc1pp5t2a3j6fl8v7785szxeyhk8dpqksas7w5ta9j8caysn5ud8l68qcey6ak")

    print("Ok")
except Exception as e:
    print("Positive P2TR failed: unknown exception: ", e)

try:
    bad_p2tr_contract = CreateInscriptionBuilder(MAINNET, INSCRIPTION)

    try:
        bad_p2tr_contract.OrdOutput(329, "bc1pp5t2a3j6fl8v7785szxeyhk8dpqksas7w5ta9j8caysn5ud8l68qcey6ak")
        print("Negative P2TR failed: no exception")
    except ContractError as e:
        print("Ok: ", e)

    try:
        bad_p2tr_contract.AuthorFee(329, "bc1pp5t2a3j6fl8v7785szxeyhk8dpqksas7w5ta9j8caysn5ud8l68qcey6ak")
        print("Negative P2TR failed: no exception")
    except ContractError as e:
        print("Ok: ", e)

    try:
        bad_p2tr_contract.MarketFee(329, "bc1pp5t2a3j6fl8v7785szxeyhk8dpqksas7w5ta9j8caysn5ud8l68qcey6ak")
        print("Negative P2TR failed: no exception")
    except ContractError as e:
        print("Ok: ", e)

    try:
        bad_p2tr_contract.AddCustomFee(329, "bc1pp5t2a3j6fl8v7785szxeyhk8dpqksas7w5ta9j8caysn5ud8l68qcey6ak")
        print("Negative P2TR failed: no exception")
    except ContractError as e:
        print("Ok: ", e)

    try:
        bad_p2tr_contract.AddUTXO("", 0, 329, "bc1pp5t2a3j6fl8v7785szxeyhk8dpqksas7w5ta9j8caysn5ud8l68qcey6ak")
        print("Negative P2TR failed: no exception")
    except ContractError as e:
        print("Ok: ", e)

except Exception as e:
    print("Negative P2TR fFailed: unknown exception: ", e)


#p2wpkh
try:
    p2wpkh_contract = CreateInscriptionBuilder(MAINNET, INSCRIPTION)
    p2wpkh_contract.OrdOutput(294, "bc1q7g2ek6p3gjlp7j639mxk95tm7f3h839mhk9v97")
    p2wpkh_contract.AuthorFee(294, "bc1q7g2ek6p3gjlp7j639mxk95tm7f3h839mhk9v97")
    p2wpkh_contract.MarketFee(294, "bc1q7g2ek6p3gjlp7j639mxk95tm7f3h839mhk9v97")
    p2wpkh_contract.AddCustomFee(294, "bc1q7g2ek6p3gjlp7j639mxk95tm7f3h839mhk9v97")

    p2wpkh_contract.AddUTXO("", 0, 294, "bc1q7g2ek6p3gjlp7j639mxk95tm7f3h839mhk9v97")

    print("Ok")
except Exception as e:
    print("Positive P2WPKH failed: unknown exception: ", e)

try:
    bad_p2wpkh_contract = CreateInscriptionBuilder(MAINNET, INSCRIPTION)

    try:
        bad_p2wpkh_contract.OrdOutput(293, "bc1q7g2ek6p3gjlp7j639mxk95tm7f3h839mhk9v97")
        print("Negative P2WPKH failed: no exception")
    except ContractError as e:
        print("Ok: ", e)

    try:
        bad_p2wpkh_contract.AuthorFee(293, "bc1q7g2ek6p3gjlp7j639mxk95tm7f3h839mhk9v97")
        print("Negative P2WPKH failed: no exception")
    except ContractError as e:
        print("Ok: ", e)

    try:
        bad_p2wpkh_contract.MarketFee(293, "bc1q7g2ek6p3gjlp7j639mxk95tm7f3h839mhk9v97")
        print("Negative P2WPKH failed: no exception")
    except ContractError as e:
        print("Ok: ", e)

    try:
        bad_p2wpkh_contract.AddCustomFee(293, "bc1q7g2ek6p3gjlp7j639mxk95tm7f3h839mhk9v97")
        print("Negative P2WPKH failed: no exception")
    except ContractError as e:
        print("Ok: ", e)

    try:
        bad_p2wpkh_contract.AddUTXO("", 0, 293, "bc1q7g2ek6p3gjlp7j639mxk95tm7f3h839mhk9v97")
        print("Negative P2WPKH failed: no exception")
    except ContractError as e:
        print("Ok: ", e)

except Exception as e:
    print("Negative P2WPKH failed: unknown exception: ", e)


#p2pkh
try:
    p2sh_contract = CreateInscriptionBuilder(MAINNET, INSCRIPTION)
    p2sh_contract.OrdOutput(546, "1PC7E8JRBw5UY8xDQUKgWLUsdRJBDTqsRe")
    p2sh_contract.AuthorFee(546, "1PC7E8JRBw5UY8xDQUKgWLUsdRJBDTqsRe")
    p2sh_contract.MarketFee(546, "1PC7E8JRBw5UY8xDQUKgWLUsdRJBDTqsRe")
    p2sh_contract.AddCustomFee(546, "1PC7E8JRBw5UY8xDQUKgWLUsdRJBDTqsRe")

    p2sh_contract.AddUTXO("", 0, 546, "1PC7E8JRBw5UY8xDQUKgWLUsdRJBDTqsRe")

    print("Ok")
except Exception as e:
    print("Positive P2PKH failed: unknown exception: ", e)

try:
    bad_p2sh_contract = CreateInscriptionBuilder(MAINNET, INSCRIPTION)

    try:
        bad_p2sh_contract.OrdOutput(545, "1PC7E8JRBw5UY8xDQUKgWLUsdRJBDTqsRe")
        print("Negative P2PKH failed: no exception")
    except ContractError as e:
        print("Ok: ", e)

    try:
        bad_p2sh_contract.AuthorFee(545, "1PC7E8JRBw5UY8xDQUKgWLUsdRJBDTqsRe")
        print("Negative P2PKH failed: no exception")
    except ContractError as e:
        print("Ok: ", e)

    try:
        bad_p2sh_contract.MarketFee(545, "1PC7E8JRBw5UY8xDQUKgWLUsdRJBDTqsRe")
        print("Negative P2PKH failed: no exception")
    except ContractError as e:
        print("Ok: ", e)

    try:
        bad_p2sh_contract.AddCustomFee(545, "1PC7E8JRBw5UY8xDQUKgWLUsdRJBDTqsRe")
        print("Negative P2PKH failed: no exception")
    except ContractError as e:
        print("Ok: ", e)

    try:
        bad_p2sh_contract.AddUTXO("", 0, 545, "1PC7E8JRBw5UY8xDQUKgWLUsdRJBDTqsRe")
        print("Negative P2PKH failed: no exception")
    except ContractError as e:
        print("Ok: ", e)

except Exception as e:
    print("Negative P2PKH failed: unknown exception: ", e)


#p2sh
try:
    p2sh_contract = CreateInscriptionBuilder(MAINNET, INSCRIPTION)
    p2sh_contract.OrdOutput(540, "34nr5Pbq53Uj2Sq5DaDw6mK63qhUgbovCf")
    p2sh_contract.AuthorFee(540, "34nr5Pbq53Uj2Sq5DaDw6mK63qhUgbovCf")
    p2sh_contract.MarketFee(540, "34nr5Pbq53Uj2Sq5DaDw6mK63qhUgbovCf")
    p2sh_contract.AddCustomFee(540, "34nr5Pbq53Uj2Sq5DaDw6mK63qhUgbovCf")

    p2sh_contract.AddUTXO("", 0, 540, "34nr5Pbq53Uj2Sq5DaDw6mK63qhUgbovCf")

    print("Ok")
except Exception as e:
    print("Positive P2SH failed: unknown exception: ", e)

try:
    bad_p2sh_contract = CreateInscriptionBuilder(MAINNET, INSCRIPTION)

    try:
        bad_p2sh_contract.OrdOutput(539, "34nr5Pbq53Uj2Sq5DaDw6mK63qhUgbovCf")
        print("Negative P2SH failed: no exception")
    except ContractError as e:
        print("Ok: ", e)

    try:
        bad_p2sh_contract.AuthorFee(539, "34nr5Pbq53Uj2Sq5DaDw6mK63qhUgbovCf")
        print("Negative P2SH failed: no exception")
    except ContractError as e:
        print("Ok: ", e)

    try:
        bad_p2sh_contract.MarketFee(539, "34nr5Pbq53Uj2Sq5DaDw6mK63qhUgbovCf")
        print("Negative P2SH failed: no exception")
    except ContractError as e:
        print("Ok: ", e)

    try:
        bad_p2sh_contract.AddCustomFee(539, "34nr5Pbq53Uj2Sq5DaDw6mK63qhUgbovCf")
        print("Negative P2SH failed: no exception")
    except ContractError as e:
        print("Ok: ", e)

    try:
        bad_p2sh_contract.AddUTXO("", 0, 539, "34nr5Pbq53Uj2Sq5DaDw6mK63qhUgbovCf")
        print("Negative P2SH failed: no exception")
    except ContractError as e:
        print("Ok: ", e)

except Exception as e:
    print("Negative P2SH failed: unknown exception: ", e)


