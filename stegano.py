import cv2
import numpy as np
import argparse as ap

def checkbit(byte,n):
    return byte&(2**(n)) != 0

def setbit(byte,n):
    return byte|(2**n)

def clearbit(byte,n):
    return byte&~(2**n)

def encode(img, string):
    bimg = bytearray(img)
    bstring = bytearray(string,'utf-8')
    offset = 8*8 #bit
    bstrlen = len(string).to_bytes(8,'big')
    for byte in range(8):
        for bit in range(8):
            isset = checkbit(bstrlen[byte],bit)
            if isset:
                bimg[byte*8 + bit] = bimg[byte*8 + bit]|1
            else:
                bimg[byte*8 + bit] = bimg[byte*8 + bit]&(~1)
    for byte in range(len(bstring)):
        for bit in range(8):
            isset = checkbit(bstring[byte],bit)
            currentbit = byte*8 + bit + offset
            if isset:
                bimg[currentbit] = bimg[currentbit]|1
            else:
                bimg[currentbit] = bimg[currentbit]&(~1)
    img2 = np.frombuffer(bimg,dtype=img.dtype).reshape(img.shape)
    return img2

def decode(img):
    bimg = bytearray(img)
    offset = 8*8
    bstrlen = bytearray(8)
    for byte in range(8):
        for bit in range(8):
            isset = checkbit(bimg[byte*8 + bit],0)
            if isset:
                bstrlen[byte] = setbit(bstrlen[byte],bit)
            else:
                bstrlen[byte] = clearbit(bstrlen[byte],bit)
    strlen = int.from_bytes(bstrlen,'big')
    bstring = bytearray(strlen)
    for byte in range(strlen):
        for bit in range(8):
            currentbit = byte*8 + bit + offset
            isset = checkbit(bimg[currentbit],0)
            if isset:
                bstring[byte] = setbit(bstring[byte],bit)
            else:
                bstring[byte] = clearbit(bstring[byte],bit)
    string = bstring.decode('utf-8')
    return string

if __name__ == "__main__":
    parser = ap.ArgumentParser()
    parser.add_argument("pic_path")
    parser.add_argument("--text")
    args = parser.parse_args()

    img = cv2.imread(args.pic_path + '.png')

    if args.text:
        #mode encode
        img_out = encode(img,args.text)
        cv2.imwrite(args.pic_path + '_encoded.png',img_out)
    else:
        #mode decode
        string_out = decode(img)
        print(string_out)

    
    # img = cv2.imread('test.png')
    # teststring = "Hello World Test!"
    # img2 = encode(img,teststring)
    # resstring = decode(img2)
    # print(resstring==teststring)