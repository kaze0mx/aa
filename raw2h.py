import sys

with open(sys.argv[1],"rb") as f:
	data=f.read()

for i in range(0,len(data),32):
    print("\"%s\" %s" % ("".join(map(lambda x:"\\x{:02x}".format(x),data[i:i+32])),i+32<len(data) and "\\" or ""))
