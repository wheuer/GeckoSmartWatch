from PIL import Image

# Image.txt should be the straight output of the C array in hex seperated by commas so still 1-bit/pixel

def convertArrayToImage():
    with open("./image.txt", "r") as file:
        values = [int(i.replace('\n', '').replace('\r', '').replace(' ', '').replace('0x', ''), 16) for i in file.read().split(',')]
        for i in range(len(values)):
            values[i] = 255 - values[i]
        image = Image.frombytes(mode="1", size=(400, 300), data=bytearray(values))
        image.show()
        image.save('./image.png')

if __name__ == "__main__":
    convertArrayToImage()

