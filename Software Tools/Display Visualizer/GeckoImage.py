from PIL import Image, ImageDraw

# Replicated C implementation should give the desired image ouput as a text file of hex values
# Each pixel is represented in 5-6-5 color, so 2 bytes/pixel
def convertArrayToImage():
    with open("./image.txt", "r") as file:
        values = [int(i.replace('\n', '').replace('\r', '').replace(' ', '').replace('0x', ''), 16) for i in file.read().split(',')]
        image_data = []
        for i in list(zip(values, values[1:]))[::2]:
            # Extract color components and scale to 0 - 255
            red = int(((i[0] >> 3) / 31) * 255) # Upper 5 bits of first byte
            green = int(((((i[0] & 0x07) << 3) | (i[1] >> 5)) / 63) * 255) # Lower 3 bits of first byte and upper 3 bits of second byte
            blue = int(((i[1] & 0x1F) / 31) * 255) # Lower 5 bits of second byte
            image_data.extend([red, green, blue])
        image = Image.frombytes(mode="RGB", size=(240, 240), data=bytearray(image_data))
        imageMod = ImageDraw.Draw(image)
        imageMod.arc([0, 0, 239, 239], 0, 360, (255, 255, 255), 1) # Make at 239 to not cut off circle, don't want to be on the edge anyway
        image.show()
        image.save('./image.png')

def createTestImage():
    with open("./image.txt", "w") as file:
        for i in range((240 * 240) - 1):
            # file.write(f"{hex(0xF8)},{hex(0x00)},") # Red
            # file.write(f"{hex(0x07)},{hex(0xE0)},") # Green
            file.write(f"{hex(0x00)},{hex(0x1F)},") # Blue
        # file.write(f"{hex(0xF8)},{hex(0x00)}") # Red
        # file.write(f"{hex(0x07)},{hex(0xE0)}") # Green
        file.write(f"{hex(0x00)},{hex(0x1F)}") # Blue

if __name__ == "__main__":
    # createTestImage()
    convertArrayToImage()

