#cython: language_level=3
import cv2
import numpy as np
import argparse as ap

def checkbit(byte,n):
    return byte&(2**(n)) != 0

def setbit(byte,n):
    return byte|(2**n)

def clearbit(byte,n):
    return byte&~(2**n)

def padstring(s,c):
    return c * (8*(len(s)//8 + 1) - (len(s)%8)) + s
    
def encode_bytes(bdest,bsrc,offset):
    for byte in range(len(bsrc)):
        for bit in range(8):
            isset = checkbit(bsrc[byte],bit)
            currentbit = (byte + offset)*8 + bit
            if isset:
                bdest[currentbit] = bdest[currentbit]|1
            else:
                bdest[currentbit] = bdest[currentbit]&(~1)
    return bdest

def decode_bytes(bsrc,length,offset):
    bdest = bytearray(length)
    for byte in range(length):
        for bit in range(8):
            currentbit = (byte + offset)*8 + bit
            isset = checkbit(bsrc[currentbit],0)
            if isset:
                bdest[byte] = setbit(bdest[byte],bit)
            else:
                bdest[byte] = clearbit(bdest[byte],bit)
    return bdest

def encode_file(bsrc,filepath,offset):
    with open(filepath,"rb") as f:
        bytes_in_file = f.read()
    bsrc = encode_bytes(bsrc,bytes_in_file,offset)
    #img2 = np.frombuffer(bimg,dtype=img.dtype).reshape(img.shape)
    return bsrc

def decode_file(bsrc,length,offset,filepath):
    with open(filepath,"wb") as f:
        bytes_in_file = f.read()
    bdest = decode_bytes(bsrc,length,offset)
    with open(filepath,'rb') as f:
        f.write(bdest)
    return

def encode_string(bdest,ssrc,offset):
    bstring = ssrc.encode('utf-8')
    bdest = encode_bytes(bdest,bstring,offset)
    return bdest

def decode_string(bsrc,length,offset):
    bstring = decode_bytes(bsrc,length,offset)
    string = bstring.decode('utf-8')
    return string

def encode_header(bimg,length,content_type,file_ending=None):
    offset_l = 0
    offset_type = 8
    offset_fileending = 16
    types = {'string':'s','file':'f'} # s -> string | f -> file
    content_offsets = {'string': 16, 'file':24}
    bimg = encode_bytes(bimg,length.to_bytes(8,'big'),0)
    encoded_type = padstring(types[content_type],'-')
    bimg = encode_string(bimg,encoded_type,offset_type)
    if content_type == 'file':
        bimg = encode_string(bimg,padstring(file_ending,'-'),offset_fileending)
    return (bimg,content_offsets[content_type])

def decode_header(bimg):
    offset_l = 0
    offset_type = 8
    offset_fileending = 16
    types = {'s':'string','f':'file'} # s -> string | f -> file
    content_offsets = {'string': 16, 'file':24}
    length = int.from_bytes(decode_bytes(bimg,8,offset_l),'big')
    content_type = decode_string(bimg,8,offset_type)
    content_type = content_type.split('-')[-1]
    content_type = types[content_type]
    content_offset = content_offsets[content_type]
    retval = {'length':length,'content_type':content_type,'content_offset':content_offset}
    if content_type == 'file':
        file_ending = decode_string(bimg,8,offset_fileending).split('-')[-1]
        retval['file_ending'] = file_ending
    return retval

def img_to_bin(img):
    if str(img.dtype) != 'utf-8' and str(img.dtype) != 'uint8':
        print("Incompatible type of pixel encoding.\n")
    return bytearray(img)

def bin_to_img(bimg,shape):
    return np.frombuffer(bimg,dtype=np.dtype('uint8')).reshape(shape)

def encode(bimg,string=None,filename=None):
    if string:
        bimg, offset = encode_header(bimg,len(string),'string')
        bimg = encode_string(bimg,args.text,offset)
    elif filename:
        file_ending = args.file.split('.')[-1]
        with open(filename,'rb') as f:
            bytes_in_file = f.read()
        bimg, offset = encode_header(bimg,len(bytes_in_file),'file',file_ending)
        bimg = encode_bytes(bimg,bytes_in_file,offset)
    return bimg

def decode(bimg):
    header = decode_header(bimg)
    out = {'header':header}
    if header['content_type'] == 'string':
        string_out = decode_string(bimg,header['length'],header['content_offset'])
        print(string_out)
        out['string'] = string_out
        return out
    elif header['content_type'] == 'file':
        bytes_to_file = decode_bytes(bimg,header['length'],header['content_offset'])
        filename = input(f"Secret Message is {header['file_ending']}-file. \nIf you would like to save the file, enter non-empty filename without extension: ")
        out['bytes'] = bytes_to_file
        if filename:
            with open(filename + '.' + header['file_ending'],'wb') as f:
                f.write(bytes_to_file)
        return out


if __name__ == "__main__":
    parser = ap.ArgumentParser()
    parser.add_argument("pic_path")
    parser.add_argument("--text")
    parser.add_argument("--file")
    args = parser.parse_args()

    img = cv2.imread(args.pic_path)
    bimg = img_to_bin(img)
    encode_mode = args.text or args.file
    if encode_mode:
        if(args.text):
            encode(bimg,string=args.text)
        elif(args.file):
            encode(bimg,filename=args.file)
    else:
        decode(bimg)

    if encode_mode:
        img_out = bin_to_img(bimg,img.shape)
        pic_ending = args.pic_path.split('.')[-1]
        cv2.imwrite(args.pic_path[:-(len(pic_ending) + 1)] + f"_encoded.{pic_ending}",img_out)
    
    # img = cv2.imread('test.png')
    # teststring = "Hello World Test!"
    # img2 = encode(img,teststring)
    # resstring = decode(img2)
    # print(resstring==teststring)