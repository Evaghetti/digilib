from sys import argv, exit
from PIL import Image
from typing import Dict, Tuple, List, Union
from os import makedirs

dataBase = argv[1]
folderOut = argv[2]
resourceFolder = argv[3]
FILE_OUTPUT_HEADER = f"{folderOut}/include/digivice/sprites.h"
FILE_OUTPUT_SOURCE = f"{folderOut}/source/digivice/sprites.c"

WIDTH_TILE  = 8
HEIGHT_TILE = 8
WIDTH_FONT  = int(WIDTH_TILE / 2)

WIDTH_SPRITE  = WIDTH_TILE * 2
HEIGHT_SPRITE = HEIGHT_TILE * 2

WIDTH_POPUP  = WIDTH_TILE * 4
HEIGHT_POPUP = HEIGHT_SPRITE

# Multiple frame animations

ANIMATION_WALK_BEGIN = 0
ANIMATION_WALK_END   = 3

ANIMATION_SLEEP_BEGIN = 3
ANIMATION_SLEEP_END   = 5

ANIMATION_HAPPY_BEGIN = 6
ANIMATION_HAPPY_END   = 8

ANIMATION_ANGRY_BEGIN = 8
ANIMATION_ANGRY_END   = 10

ANIMATION_SHOOTING_BEGIN = 10
ANIMATION_SHOOTING_END   = 12

ANIMATION_EATING_BEGIN = 12
ANIMATION_EATING_END   = 14

# One frame animations

ANIMATION_REFUSAL_BEGIN = 5
ANIMATION_REFUSAL_END   = 6

ANIMATION_SICK_BEGIN   = 14
ANIMATION_SICK_END     = 16

TILE_INDEX = 16

MASK_INVERTED     = 0b1000000000000000
MASK_INDEX_SPRITE = 0b0111111111111111

def createDirAndOpenFile(path: str, mode: str = "w"):
    if path.find("/") != -1:
        try:
            makedirs("/".join(path.split("/")[:-1]))
        except FileExistsError:
            pass

    return open(path, mode)


def reverseBits(bits: int) -> int:
    result = 0b00000000
    for i in range(8):
        result |= (bits & 1) << (7 - i)
        bits >>= 1
    return result

def getBinaryValuesTile(tile: Image.Image) -> List[int]:
    width, height = tile.size

    lines = []
    for y in range(height):
        currentLine = 0b00000000
        for x in range(width):
            r, g, b, a = tile.getpixel((x, y))
            currentLine |= (1 if a != 0 else 0) << (WIDTH_TILE - 1 - x)
        
        lines.append(currentLine)
    return lines

def getTilesIndicesSprite(sprite: Image.Image, isFont: bool) -> Tuple[List[str], List[int]]:
    sprite = sprite.convert("RGBA")

    width, height = sprite.size

    stepWidth = WIDTH_FONT if isFont else WIDTH_TILE

    indices = []
    values = []

    for y in range(0, height, HEIGHT_TILE):
        for x in range(0, width, stepWidth):
            tile = sprite.crop((x, y, x + stepWidth, y + HEIGHT_TILE))
            binaryValuesTile = getBinaryValuesTile(tile)
            reversedBinaryValuesTile = [reverseBits(i) for i in binaryValuesTile]

            try:
                stringfiedBinary = convertBinaryValuesToString(binaryValuesTile)
                currentIndex = values.index(stringfiedBinary)
            except ValueError:
                try:
                    stringfiedBinary = convertBinaryValuesToString(reversedBinaryValuesTile)
                    currentIndex = values.index(stringfiedBinary)
                    currentIndex |= MASK_INVERTED
                except ValueError:
                    currentIndex = len(values)
                    stringfiedBinary = convertBinaryValuesToString(binaryValuesTile)
            
            if currentIndex == len(values):
                values.append(stringfiedBinary)
            indices.append(currentIndex)

    return values, indices

def convertBinaryValuesToString(binaryValues: List[int]) -> str:
    lines = ""
    for i in binaryValues:
        currentLine = ""
        for _ in range(WIDTH_TILE):
            carry, i = (i & 0b10000000) >> 7, i << 1
            currentLine += str(carry)
        lines += f"0b{currentLine},\n"
    
    return lines[:-1]

def addTilesWithoutDuplicates(inTiles: List[str], inIndices: List[int], outTiles: List[str], outIndices: List[List[int]]):
    updated = []
    for i, stringSprite in enumerate(inTiles):
        if not stringSprite in outTiles:
            indexToUpdate = len(outTiles)

            outTiles.append(stringSprite)
        else:
            indexToUpdate = outTiles.index(stringSprite)

        position = -1
        while True:
            try:
                if i in inIndices:
                    position = inIndices.index(i, position + 1)
                else:
                    position = inIndices.index(i | MASK_INVERTED, position + 1)
                    
                if position in updated:
                    continue
                else:
                    updated.append(position)

                isInverted = inIndices[position] & MASK_INVERTED
                inIndices[position] = indexToUpdate | isInverted
            except ValueError:
                break
    outIndices.append(inIndices)

def addTilesWithoutDuplicatesGlobal(inTiles: List[str], inIndiceDatabase: List[List[int]], outTiles: List[str], outIndices: List[List[int]]):
    for i in inIndiceDatabase:
        addTilesWithoutDuplicates(inTiles, i, outTiles, outIndices)

def parseImage(pathImage: str, widthSprite: int, heightSprite: int, shouldRemoveIndices: bool) -> Tuple[List[str], List[List[int]]]:
    image: Image.Image = Image.open(pathImage)
    
    width, height = image.size

    tileSheetImage, indicesSpriteImage = [], []

    for y in range(0, height, heightSprite):
        for x in range(0, width, widthSprite):
            sprite = image.crop((x, y, x + widthSprite, y + heightSprite))
            tiles, indices = getTilesIndicesSprite(sprite, widthSprite < WIDTH_TILE)
            addTilesWithoutDuplicates(tiles, indices, tileSheetImage, indicesSpriteImage)
    if shouldRemoveIndices:
        indicesSpriteImage = indicesSpriteImage[:-1]
        indicesSpriteImage[-1] = [indicesSpriteImage[-1][0]]
    return tileSheetImage, indicesSpriteImage

def removeDuplicateIndices(indices: List[List[int]]) -> List[Union[List[int], int]]:
    result: List[Union[List[int], int]] = [i for i in indices]

    for i, index in enumerate(result):
        if isinstance(index, int):
            continue
        
        position = i
        while True:
            try:
                position = result.index(index, position + 1)
                result[position] = i
            except ValueError:
                break
    return result

def getTransformedIndex(index: int) -> int:
    return ((index & MASK_INDEX_SPRITE) * 8) | (index & MASK_INVERTED)

def getCountTile(tileDataBase: List[str]) -> int:
    return sum([tile.count(',') for tile in tileDataBase])

def getPointerToTileFromIndices(spriteDataBase: List[Union[List[int], int]]) -> List[Union[List[int], int]]:
    result: List[Union[List[int], int]] = []
    for sprite in spriteDataBase:
        if isinstance(sprite, int):
            # Currently a repeated sprite, so sprite will be an offset to the actual variable
            result.append(sprite)
        else:
            tiles = []
            for tile in sprite:
                tiles.append(getTransformedIndex(tile))
            result.append(tiles)
    return result

def getDigimonNameAsVariable(digimonName: str) -> str:
    return digimonName.title().replace(" ", "")

def getVariablesAndDeclarations(digimonSprites: Dict[str, List[Union[List[int], int]]], digimonName: str):
    variables, declarations = [], []
    count = 0

    digimonAnimation = getPointerToTileFromIndices(digimonSprites[digimonName])
    for animation in digimonAnimation:
        if isinstance(animation, int):
            variables.append(variables[animation])
            continue

        currentVariableContent = ','.join(str(i) for i in animation)
    
        variableName = f"guiSpriteTileIndex{getDigimonNameAsVariable(digimonName)}{count}"
        declaration = f"const uint16_t {variableName}[] = {{{currentVariableContent}}};"
        
        variables.append(f"{variableName}")
        declarations.append(declaration)
        
        count += 1
    return variables, declarations

def writeAnimation(animationDataBase, begin: int, end: int, out):
    print("{", end="", file=out) # Animation
    print(",".join(animationDataBase[begin:end]), end="", file=out)
    print("},", file=out)

def getPointer(indices: List[List[int]]) -> List[str]:
    result = []
    for i in indices:
        result += [f"&guiTileDatabase[{getTransformedIndex(j)}]" for j in i]
    return result

def createAnimationArray(name: str, indices: List[List[int]]) -> str:
    indicesToWrite = getPointer(indices) 

    output = f"const uint8_t *const gui{name.capitalize()}Animation[MAX_FRAMES_ANIMATION] = " + "{"
    output += ",".join(indicesToWrite)
    output += "};"

    return output

def main():
    spriteDataBase = {}
    tileDatabase, indiceDatabase = [], []
    with open(dataBase, "r") as file:
        next(file) # first line is just a comment

        for line in file:
            digimonData = line.split(";")
            digimonName = digimonData[0].lower()
            digimonStage = digimonData[6].lower()
            
            print(digimonName)
            digimonTiles, digimonIndices = parseImage(f"{resourceFolder}/{digimonName}.gif", WIDTH_SPRITE, HEIGHT_SPRITE, digimonStage != "egg")
            addTilesWithoutDuplicatesGlobal(digimonTiles, digimonIndices, tileDatabase, indiceDatabase)
            spriteDataBase[digimonName] = removeDuplicateIndices(digimonIndices)
            print("Done")

        print("Finished reading digimon images digimons")

    tilesFeed, feedIndices = parseImage(f"{resourceFolder}/feed.gif", WIDTH_TILE, HEIGHT_TILE, False)
    addTilesWithoutDuplicatesGlobal(tilesFeed, feedIndices, tileDatabase, indiceDatabase)
    print("Finished reading digimon images feed")

    tilesPopup, popupIndices = parseImage(f"{resourceFolder}/popups.gif", WIDTH_POPUP, HEIGHT_POPUP, False)
    addTilesWithoutDuplicatesGlobal(tilesPopup, popupIndices, tileDatabase, indiceDatabase)
    print("Finished reading popups")

    tilesFont, fontIndices = parseImage(f"{resourceFolder}/font.png", WIDTH_FONT, HEIGHT_TILE, False)
    print("Finished preparing font")

    print("Done")
    

    with createDirAndOpenFile(FILE_OUTPUT_HEADER) as outHeader:
        print("#ifndef SPRITES_H", file=outHeader)
        print("#define SPRITES_H\n", file=outHeader)

        print("#include \"digitype.h\"", file=outHeader)
        print("#include \"digiworld.h\"\n", file=outHeader)

        print("#define GET_INDEX_TILE(x)   (x & 0b0111111111111111)", file=outHeader)
        print("#define IS_TILE_INVERTED(x) ((x & 0b1000000000000000) != 0)\n", file=outHeader)

        print(f"#define COUNT_TILES {getCountTile(tileDatabase)}", file=outHeader)
        print(f"#define COUNT_FONT {getCountTile(tilesFont)}", file=outHeader)
        print(f"#define FIRST_CHARACTER ' '", file=outHeader)

        print(f"#define MAX_COUNT_ANIMATIONS             6", file=outHeader)
        print(f"#define MAX_COUNT_SINGLE_FRAME_ANIMATION 1", file=outHeader)
        print(f"#define MAX_FRAMES_ANIMATION_WALKING     3", file=outHeader)
        print(f"#define MAX_FRAMES_ANIMATION             2", file=outHeader)
        print(f"#define MAX_TILE_POPUPS                  8\n", file=outHeader)

        print(f"#define MAX_COUNT_EATING_ANIMATIONS       2", file=outHeader)
        print(f"#define MAX_FRAMES_EATING_ANIMATIONS      4\n", file=outHeader)

        print(f"#define SELECTOR_TILE     &guiTileDatabase[{getTransformedIndex(feedIndices[-4][0])}]\n", file=outHeader)
        print(f"#define CLEANING_TILE     &guiTileDatabase[{getTransformedIndex(feedIndices[16][0])}]\n", file=outHeader)
        print(f"#define HAPPY_SUN_TILE    &guiTileDatabase[{getTransformedIndex(feedIndices[ 8][0])}]\n", file=outHeader)
        print(f"#define EMPTY_HEART_TILE  &guiTileDatabase[{getTransformedIndex(feedIndices[21][0])}]\n", file=outHeader)
        print(f"#define FILLED_HEART_TILE &guiTileDatabase[{getTransformedIndex(feedIndices[20][0])}]\n", file=outHeader)
        print(f"#define AGE_INFO_TILE     &guiTileDatabase[{getTransformedIndex(feedIndices[18][0])}]\n", file=outHeader)
        print(f"#define SCALE_INFO_TILE   &guiTileDatabase[{getTransformedIndex(feedIndices[19][0])}]\n", file=outHeader)
        print(f"#define SHIELD_TILE       &guiTileDatabase[{getTransformedIndex(feedIndices[17][0])}]\n", file=outHeader)
        
        print(f"extern const uint8_t guiFontDatabase[COUNT_FONT];", file=outHeader)
        print(f"extern const uint8_t guiTileDatabase[COUNT_TILES];", file=outHeader)
        print(f"extern const uint16_t *const guiDigimonAnimationDatabase[MAX_COUNT_DIGIMON][MAX_COUNT_ANIMATIONS][MAX_FRAMES_ANIMATION];", file=outHeader)
        print(f"extern const uint16_t *const guiDigimonWalkingAnimationDatabase[MAX_COUNT_DIGIMON][MAX_FRAMES_ANIMATION_WALKING];", file=outHeader)
        print(f"extern const uint16_t *const guiDigimonSingleFrameAnimationDatabase[MAX_COUNT_DIGIMON][MAX_COUNT_SINGLE_FRAME_ANIMATION];", file=outHeader)
        print(f"extern const uint8_t *const guiDigimonProjectileSprites[MAX_COUNT_DIGIMON];", file=outHeader)
        print(f"extern const uint8_t *const guiFeedingAnimations[MAX_COUNT_EATING_ANIMATIONS][MAX_FRAMES_EATING_ANIMATIONS];", file=outHeader)
        print(f"extern const uint8_t *const guiSnoreAnimation[MAX_FRAMES_ANIMATION];", file=outHeader)
        print(f"extern const uint8_t *const guiPoopAnimation[MAX_FRAMES_ANIMATION];", file=outHeader)
        print(f"extern const uint8_t *const guiSkullAnimation[MAX_FRAMES_ANIMATION];", file=outHeader)
        print(f"extern const uint8_t *const guiStormAnimation[MAX_FRAMES_ANIMATION];\n", file=outHeader)

        print(f"extern const uint8_t *const guiDamagePopuoAnimation[MAX_FRAMES_ANIMATION][MAX_TILE_POPUPS];", file=outHeader)
        print(f"extern const uint8_t *const guiBattlePopupSprite[MAX_TILE_POPUPS];", file=outHeader)

        print("\n#endif // SPRITES_H", file=outHeader)

    with createDirAndOpenFile(FILE_OUTPUT_SOURCE) as outSource:
        print("#include \"sprites.h\"\n", file=outSource)

        print("const uint8_t guiTileDatabase[COUNT_TILES] = {", file=outSource)
        print("\n".join(tileDatabase), file=outSource)
        print("};\n", file=outSource)

        print("const uint8_t guiFontDatabase[COUNT_FONT] = {", file=outSource)
        print("\n".join(tilesFont), file=outSource)
        print("};\n", file=outSource)

        animationDatabase = []
        for digimonName in spriteDataBase.keys():
            variables, declarations = getVariablesAndDeclarations(spriteDataBase, digimonName)
            animationDatabase.append(variables)
            print("\n".join(declarations), file=outSource)

        print("\nconst uint16_t *const guiDigimonAnimationDatabase[MAX_COUNT_DIGIMON][MAX_COUNT_ANIMATIONS][MAX_FRAMES_ANIMATION] = {", file=outSource)
        for i, digimonName in enumerate(spriteDataBase.keys()):
            if len(spriteDataBase[digimonName]) <= 3:
                continue

            print("{", file=outSource) # Digimon
            writeAnimation(animationDatabase[i], ANIMATION_SLEEP_BEGIN, ANIMATION_SLEEP_END, outSource)
            writeAnimation(animationDatabase[i], ANIMATION_HAPPY_BEGIN, ANIMATION_HAPPY_END, outSource)
            writeAnimation(animationDatabase[i], ANIMATION_ANGRY_BEGIN, ANIMATION_ANGRY_END, outSource)
            writeAnimation(animationDatabase[i], ANIMATION_SHOOTING_BEGIN, ANIMATION_SHOOTING_END, outSource)
            writeAnimation(animationDatabase[i], ANIMATION_EATING_BEGIN, ANIMATION_EATING_END, outSource)
            writeAnimation(animationDatabase[i], ANIMATION_SICK_BEGIN, ANIMATION_SICK_END, outSource)
            
            print("}", end=",\n", file=outSource)
        print("};", file=outSource)

        print("\nconst uint8_t *const guiDigimonProjectileSprites[MAX_COUNT_DIGIMON] = {", file=outSource)
        for i, digimonName in enumerate(spriteDataBase.keys()):
            if len(spriteDataBase[digimonName]) <= 3:
                continue
            
            index = getTransformedIndex(spriteDataBase[digimonName][16][0])
            print(f"&guiTileDatabase[{index}],\n", file=outSource)
        print("};", file=outSource)

        print("\nconst uint16_t *const guiDigimonWalkingAnimationDatabase[MAX_COUNT_DIGIMON][MAX_FRAMES_ANIMATION_WALKING] = {", file=outSource)
        for i, digimonName in enumerate(spriteDataBase.keys()):            
            writeAnimation(animationDatabase[i], ANIMATION_WALK_BEGIN, ANIMATION_WALK_END, outSource)
        print("};", file=outSource)

        print("\nconst uint16_t *const guiDigimonSingleFrameAnimationDatabase[MAX_COUNT_DIGIMON][MAX_COUNT_SINGLE_FRAME_ANIMATION] = {", file=outSource)
        for i, digimonName in enumerate(spriteDataBase.keys()):
            if len(spriteDataBase[digimonName]) <= 3:
                continue

            print("{", end="", file=outSource)
            print(animationDatabase[i][ANIMATION_REFUSAL_BEGIN], sep=",", end="", file=outSource)            
            print("},", end="\n", file=outSource)
        print("};", file=outSource)

        # TODO: Organize this better
        print("\nconst uint8_t *const guiFeedingAnimations[MAX_COUNT_EATING_ANIMATIONS][MAX_FRAMES_EATING_ANIMATIONS] = {", file=outSource)
        indicesFeedToWrite = getPointer(feedIndices[0:4])
        print("{", ",".join(indicesFeedToWrite), "},", file=outSource)
        indicesFeedToWrite = getPointer(feedIndices[4:8])
        print("{", ",".join(indicesFeedToWrite), "}", file=outSource)
        print("};", file=outSource)

        print(createAnimationArray("snore", feedIndices[11:13]), file=outSource) 
        print(createAnimationArray("poop", feedIndices[9:11]), file=outSource) 
        print(createAnimationArray("skull", feedIndices[13:15]), file=outSource) 
        print(createAnimationArray("storm", feedIndices[23:25]), file=outSource) 

        indicesDamagePopup = [getPointer([popupIndices[0]]),getPointer([popupIndices[1]])] 
        print("const uint8_t *const guiDamagePopuoAnimation[MAX_FRAMES_ANIMATION][MAX_TILE_POPUPS] = {", file=outSource)
        print("{", ",".join(indicesDamagePopup[0]), "},", file=outSource)
        print("{", ",".join(indicesDamagePopup[1]), "}", file=outSource)
        print("};", file=outSource)

        indicesBattlePopup = getPointer([popupIndices[2]]) 
        print("const uint8_t *const guiBattlePopupSprite[MAX_TILE_POPUPS] = {", file=outSource)
        print(",".join(indicesBattlePopup), file=outSource)
        print("};", file=outSource)

if __name__ == "__main__":
    exit(main())
