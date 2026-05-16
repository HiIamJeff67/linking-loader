import objreader

def execute(argv, ESTAB):
    PROGADDR = int ("%s" % argv[1], 16)
    CSADDR = PROGADDR
        
    memoryspacelen = 0
    
    for i in range(2, len(argv)):
        lines = objreader.readfile(argv[i])
        # Header Record
        csname = objreader.getNameWithoutSpace(lines[0][1:7])
        cslength = int("%s" % lines[0][13:19], 16)
        
        memoryspacelen += cslength
        
        ESTAB[csname] = CSADDR
        
        for l in range(1, len(lines)):
            if lines[l][0:1] != 'D':
                continue
            # D Record
            n = int((len(lines[l])-2)/12)
            for j in range(0, n):
                name = objreader.getNameWithoutSpace(lines[l][1+(12*j):7+(12*j)])
                addr = int("%s" % lines[l][7+(12*j):13+(12*j)], 16)
                ESTAB[name] = CSADDR + addr
                
        CSADDR += cslength
        
    return memoryspacelen

        
    
    
    
    
    
    
    
