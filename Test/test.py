import os
import subprocess
def judge(item):
    with open("./Test/{}/{}.out".format(item,item),"r") as f:
        standardRes=f.read().strip()
    cmd =  "./bin/deepvm ./Test/{}/{}.wasm".format(item,item)
    p = subprocess.Popen(cmd.split(), shell=False, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    p.wait(6)
    res=bytes.decode(p.stdout.read().strip())
    print("---------------------------------")
    print(res)
    print("---------------------------------")
    if standardRes == res :
        print("{}:passed".format(item))
        return True
    else :
        print("standard result is {}".format(standardRes))
        print("{}:failed".format(item))
        return False
        


def check(testList):
    pCount=0
    fCount=0 
    for i in testList:
        print("\n\nChecking {}".format(i))
        if judge(i):
            pCount+=1
        else:
            fCount+=1
    print("\n\nTest finished!")
    print("Passed:{}".format(pCount))
    print("Failed:{}".format(fCount))
    
if __name__ == "__main__":
    testList = []
    with open('./Test/testList.txt','r') as f:
        for i in f.readlines():
            testList.append(i.strip())
    if len(testList)==0:
        for root,dirs,file in os.walk("./Test",topdown=False):
            for name in dirs:
                testList.append(name)
    print(testList)
    check(testList)


