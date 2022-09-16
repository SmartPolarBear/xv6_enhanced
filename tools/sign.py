import sys


def main(argv):
    file=str(argv[1])

    bytes=[]
    with open(file, "rb") as f:
        bytes=f.read()
    
    print(f"File length: {len(bytes)}")

    bytes=list(bytes)

    if len(bytes) > 510 or len(bytes) <= 0:
        print(f"Invalid length (0<l<=510) {len(bytes)}")
        exit(1)
    
    for i in range(510-len(bytes)):
        bytes.append(0)

    bytes.append(0x55)
    bytes.append(0xAA)

    with open(file, "wb") as f:
        f.write(bytearray(bytes))

if __name__ == "__main__":
    main(sys.argv)