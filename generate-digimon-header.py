from sys import argv

fileName = argv[1]
folderOut = argv[2]
FILE_OUTPUT = f"{folderOut}/include/digiworld.h"
FILE_OUTPUT_SOURCE = "source/digiworld.c"

NAME = 0
SLOT_OR_POWER = 1
HOUR_TO_WAKE_UP = 2
HOUR_TO_SLEEP = 3
ATTRIBUTE = 4
TIME_TO_EVOLVE = 5
STAGE = 6
VERSION = 7
END_DIGIMON = VERSION + 1

NAME_TARGET = 0
PROGRESSION_NEEDED = 1
CARE_MISTAKES_COUNT = 2
TRAINING_COUNT = 3
OVERFEEDING_COUNT = 4
SLEEP_DISTURBANCE_COUNT = 5
WIN_COUNT = 6
END_REQUIREMENT = WIN_COUNT + 1

PROGRESSION_CARE_MISTAKES = 0b00000001
PROGRESSION_TRAINING = 0b00000010
PROGRESSION_OVERFEED = 0b00000100
PROGRESSION_WIN_COUNT = 0b00001000
PROGRESSION_SLEEP_DISTURBANCE = 0b00010000


def getMin(value):
    return value >> 8


def getMax(value):
    return value & 0b11111111


class Digimon:
    def __init__(self, data: list):
        self.name = data[NAME]
        self.slotPower = int(data[SLOT_OR_POWER])
        self.hourToWakeUp = int(data[HOUR_TO_WAKE_UP])
        self.hourToSleep = int(data[HOUR_TO_SLEEP])
        self.attribute = f"DIGI_ATTRIBUTE_{data[ATTRIBUTE].upper()}"
        self.timeToEvolve = int(data[TIME_TO_EVOLVE])
        self.stage = f"DIGI_STAGE_{data[STAGE].upper()}"
        self.version = int(data[VERSION])
        self.evolutionRequirements = []

        print(f"New Digimon: {self.name} ({self.attribute}/{self.stage}), version {self.version}")
        print(
            f"Its slot/power is {self.slotPower}, it takes {self.timeToEvolve} minutes to evolve")
        print(
            f"It will wake up at {int(self.hourToWakeUp / 60)}:{self.hourToWakeUp % 60} and sleep at {int(self.hourToSleep / 60)}:{self.hourToSleep % 60}")

    def getListRequirements(self) -> str:
        ret = "{"
        for i in self.evolutionRequirements:
            ret += f"&vstPossibleRequirements[{i}],"
        ret += "}"
        return ret

    def __str__(self) -> str:
        return f"{{\"{self.name}\", {self.slotPower}, {hex(self.hourToWakeUp)}, {hex(self.hourToSleep)}, {self.attribute}, {self.timeToEvolve}, {self.stage}, {self.version}, {len(self.evolutionRequirements)}, {self.getListRequirements()}}}"


class EvolutionRequirement:
    def __init__(self, indexTarget: int, data: list):
        self.indexTarget = indexTarget
        self.progression = int(data[PROGRESSION_NEEDED])
        self.careMistakes = int(data[CARE_MISTAKES_COUNT])
        self.trainingCount = int(data[TRAINING_COUNT])
        self.overFeedingCount = int(data[OVERFEEDING_COUNT])
        self.sleepDisturbanceCount = int(data[SLEEP_DISTURBANCE_COUNT])
        self.winCount = int(data[WIN_COUNT])

        print(f"Requerimento para evolução pra {data[NAME_TARGET]}:")
        if self.progression & PROGRESSION_CARE_MISTAKES:
            print(
                f"Care mistakes -> {getMin(self.careMistakes)} - {getMax(self.careMistakes)}")
        if self.progression & PROGRESSION_TRAINING:
            print(
                f"Training Count -> {getMin(self.trainingCount)} - {getMax(self.trainingCount)}")
        if self.progression & PROGRESSION_OVERFEED:
            print(
                f"Overfeeding -> {getMin(self.overFeedingCount)} - {getMax(self.overFeedingCount)}")
        if self.progression & PROGRESSION_SLEEP_DISTURBANCE:
            print(
                f"Sleep Disturbance -> {getMin(self.sleepDisturbanceCount)} - {getMax(self.sleepDisturbanceCount)}")
        if self.progression & PROGRESSION_WIN_COUNT:
            print(
                f"Win Count -> {getMin(self.winCount)} - {getMax(self.winCount)}")

    def __str__(self) -> str:
        return f"{{{self.progression}, {hex(self.careMistakes)}, {hex(self.trainingCount)}, {hex(self.overFeedingCount)}, {hex(self.sleepDisturbanceCount)}, {hex(self.winCount)}, {self.indexTarget}}}"


if __name__ == "__main__":
    digimons = []
    evolutionsRequirements = []
    evolutionRequirementsData = []

    with open(fileName, "r") as file:
        file.readline()

        for line in file:
            allData = line.split(';')

            digimons.append(Digimon(allData[:END_DIGIMON]))
            evolutionRequirementsData.append(allData[END_DIGIMON:])
        for i in range(len(digimons)):
            digimon = digimons[i]
            evolutionData = evolutionRequirementsData[i]

            for i in range(0, len(evolutionData), END_REQUIREMENT):
                nameTarget = evolutionData[i + NAME_TARGET]
                if len(nameTarget) == 0:
                    break

                for j in range(len(digimons)):
                    if digimons[j].name == nameTarget:
                        break

                evolutionsRequirements.append(
                    EvolutionRequirement(j, evolutionData[i: i + END_REQUIREMENT]))

                digimon.evolutionRequirements.append(
                    len(evolutionsRequirements) - 1)

    with open(FILE_OUTPUT, "w") as outFile:
        print("#ifndef DIGIWORLD_H\n#define DIGIWORLD_H\n", file=outFile)
        print("#include \"digimon.h\"", file=outFile)

        print(f"#define MAX_COUNT_DIGIMON {len(digimons)}", file=outFile)
        print(
            f"#define MAX_COUNT_EVOLUTION_REQUIREMTNS {len(evolutionsRequirements)}\n", file=outFile)

        print(
            f"evolution_requirement_t vstPossibleRequirements[MAX_COUNT_EVOLUTION_REQUIREMTNS];\n", file=outFile)

        print(
            f"digimon_t vstPossibleDigimon[MAX_COUNT_DIGIMON];\n", file=outFile)

        print("#endif\n", file=outFile)

    with open(FILE_OUTPUT_SOURCE, "w") as outFile:
        print("#include \"digiworld.h\"", file=outFile)
        print("#include \"digimon.h\"", file=outFile)
        print("#include \"enums.h\"\n", file=outFile)

        print(
            f"evolution_requirement_t vstPossibleRequirements[] = {{{', '.join([str(i) for i in evolutionsRequirements])}}};\n", file=outFile)
        print(
            f"digimon_t vstPossibleDigimon[] = {{{', '.join([str(i) for i in digimons])}}};\n", file=outFile)
