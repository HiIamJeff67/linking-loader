def hexToString(hexstr, length):
    hexstr = hexstr.upper()
    hexstr = hexstr[2:]
    n = length - len(hexstr)
    for i in range(0, n):
        hexstr = '0' + hexstr
    return hexstr
    
def getDec(hexstr, bits):
    intval = int(hexstr, 16)
    if intval >= int(pow(2, bits-1)):
        intval -= int(pow(2, bits))
    return intval
    
def getHex(val, bits):
    return hex((val + (1 << bits)) % (1 << bits))

    
def testMemoryContent(memorycontent):
    memorycontent[5] = 'H'
