def readfile(srcfile):
    try:
        with open(srcfile, "r") as fp:
            return fp.readlines()
    except:
            return None
    
    
def getNameWithoutSpace(originalname):
    list = originalname.split()
    return list[0]

    

