#!/usr/bin/env python

from PIL import Image
from bitarray import bitarray
import sys, getopt
import os

def print_help():
  print ('Converts an RGB JPG, PNG or BMP image to a byte array of 3-bit RBG data in a c header file')
  print ('e.g: img2rgb3header.py -i myImage.png -o myHeaderFile.h')
  print ('img2rgb3header.py -i <inputfile> -o <outputfile>')

def main(argv):
  inFilename = ''
  outFilename = ''

  # Parse command line arguments
  try:
    opts, args = getopt.getopt(argv,"hi:o:",["ifile=","ofile="])
  except getopt.GetoptError:
    print_help()
    sys.exit(2)
  for opt, arg in opts:
    if opt == '-h':
      print_help()
      sys.exit()
    elif opt in ("-i", "--ifile"):
      inFilename = arg
    elif opt in ("-o", "--ofile"):
      outFilename = arg

  inFile = Image.open(inFilename) 
  
  pixelMatrix = inFile.load()  

  width = inFile.size[0]
  height = inFile.size[1]

  print ("Opened %s" % inFilename)
  print ("Width: %d pixels" % width)
  print ("Height: %d pixels" % height)

  bits     = bitarray(0)
  outBytes = bytearray(0)

  # Add MSB of each byte to the bit array
  for y in range(0, height):
    for x in range(0, width):
      for colorByte in pixelMatrix[x,y]:
        bits.append(colorByte>>7)

  # Write bits in bitarray to byte array
  byteNum = -1;
  bitPos = 0;
  for bitNum in range (0, len(bits)):
    bitPos = bitNum%8  

    if (bitPos == 0):
      byteNum += 1
      outBytes.append(0)

    outBytes[byteNum] |= bits[bitNum]<<bitPos

  bitmapName = os.path.splitext(outFilename)[0]

  print ("Transformed RGB data from 24-bit to 3-bit")

  # Create output file and write bytes as C array
  outFile = open(outFilename, "w")
  outFile.write("#ifndef %s_BITMAP_H\n" % (bitmapName.upper()) )
  outFile.write("#define %s_BITMAP_H\n\n" % (bitmapName.upper()) )
  outFile.write("#define %s_BITMAP_WIDTH %d\n"  % (bitmapName.upper(),width) )
  outFile.write("#define %s_BITMAP_HEIGHT %d\n\n" % (bitmapName.upper(),height) )
  outFile.write("static const uint8_t %sBitmap[%d] = {" % (bitmapName, len(outBytes)))

  # Split rows before 80 characters
  for byteNum in range (0, len(outBytes)):
    if (byteNum%12 == 0):
      outFile.write("\n  ")
    if (byteNum != len(outBytes)-1):
        outFile.write("0x%02x, " %outBytes[byteNum])
    else:
        outFile.write("0x%02x" %outBytes[byteNum])
    
  outFile.write("};\n\n")
  outFile.write("#endif /* %s_BITMAP_H */\n" % (bitmapName.upper()))

  print ("Wrote %s" % outFilename)
  outFile.close()

if __name__ == "__main__":
   main(sys.argv[1:])
