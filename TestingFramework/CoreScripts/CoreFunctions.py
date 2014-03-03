import os
import time
import datetime
from ConfigParser import ConfigParser

CONFIG_FILE = "Parameters.conf"
DELIMITER = ";"


class NumberComparisonResult:
    SAME = "same"
    EQUAL = "equal"
    NOT_EQUAL = "NotEqual"

def GetTimeStamp():
    return datetime.datetime.now().strftime("%Y-%m-%d %H-%M-%S-%f")


def Absolutize(x):
    divisor = x[0]
    if divisor != 0.0:
        for i in range(0, len(x)):
            x[i] = x[i] / divisor
    else:
        for i in range(0, len(x)):
            x[i] = 0.0
    return x


def PrintAbsValues(po, sequence):
    if len(sequence):
        po.write(("%s;%s;%s" % (str(Absolutize(sequence)).replace(", ", ";")[1:-1], str(min(sequence)),
                                str(Average(sequence)))).replace(".", ","))
        po.write(2 * ";")


def Average(values):
    """Computes the arithmetic mean of a list of numbers.
    >>> print(Average([20, 30, 70]))
    40.0
    """
    return sum(values) / len(values)


def RemoveDir(dirName):
    RemovePermissions(dirName)
    for name in os.listdir(dirName):
        file = os.path.join(dirName, name)
        if not os.path.islink(file) and os.path.isdir(file):
            RemoveDir(file)
        else:
            RemovePermissions(file)
            os.remove(file)
    try:
        os.rmdir(dirName)
    except WindowsError:
        pass


def RemovePermissions(filePath):
    if not os.access(filePath, os.W_OK):
        os.chmod(filePath, 666)


def CompareValues(value1, value2, eps=0.012):
    if value1 == value2:
        return NumberComparisonResult.SAME

    value1 = float(value1)
    value2 = float(value2)

    relative_eps = min(value1, value2) * eps

    if abs(value1 - value2) < relative_eps:
        return NumberComparisonResult.EQUAL

    return NumberComparisonResult.NOT_EQUAL


def ReportErrorAndExit(error, logger):
    logger.Log(error)


def MarkResultAsBest(col):
    return "* " + col

def WriteStringToFile2(cols, tableFileName):
    printStr = ""

    """
    ";".join(cols)
    """

    for col in cols:
        if col == END_OF_COLUMN:
            printStr += ";"
        else:
            printStr += str(col)

    printStr += "\n"
    resultFile = open(tableFileName, 'a')
    resultFile.write(printStr.replace(".", ","))
    resultFile.close()


def WriteStringToFile(cols, tableFileName):
    #printStr = "%s\n" % (DELIMITER.join(cols))
    printStr = ""
    for value in cols:
        printStr += "%s%s" % (value, DELIMITER)

    printStr = printStr[:-1]
    printStr += "\n"
    resultFile = open(tableFileName, 'a')
    resultFile.write(printStr.replace(".", ","))
    resultFile.close()


def MakeTableInPercents(table):
    currStageIdx = 0

    for stage in table:
        for col in range(len(stage)):
            if (currStageIdx > 0) and (table[0][col] > 0):
                table[currStageIdx][col] = stage[col] * 100 / table[0][col]

        currStageIdx += 1

    return table


def ExtractXYFromTable(table):
    xValues = []
    yValues = []

    for currStage in table:
        xValues.append(currStage[1])
        yValues.append(currStage[0])

    return xValues, yValues


def PrintTableToFile2(tableFileName, table, metrics, stages=None):
    if not stages: stages = []
    cols = ["stage", END_OF_COLUMN]

    for col in metrics:
        cols.extend([col, END_OF_COLUMN])

    WriteStringToFile(cols, tableFileName)
    currStageIdx = 0

    for currStage in table:
        currStageName = str(currStageIdx)

        if stages:
            currStageName = stages[currStageIdx]

        cols = [currStageName, END_OF_COLUMN]

        for value in currStage:
            cols.extend([str(value), END_OF_COLUMN])

        WriteStringToFile(cols, tableFileName)
        currStageIdx += 1


def PrintTableToFile(tableFileName, table, metrics, stages=None):
    if not stages: stages = []
    cols = ["stage"]
    cols.extend(metrics)
    WriteStringToFile(cols, tableFileName)
    currStageIdx = 0

    for currStage in table:
        currStageName = str(currStageIdx)

        if stages:
            currStageName = stages[currStageIdx]

        cols = [currStageName]
        cols.extend(currStage)
        WriteStringToFile(cols, tableFileName)
        currStageIdx += 1


def CreateConfigParser():
    cfgParser = ConfigParser()
    parentDir = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir))
    configFile = os.path.join(parentDir, CONFIG_FILE)
    cfgParser.read(configFile)
    return cfgParser


def DeleteOldLogs(logsDir, daysToStoreLogs=7):
    if not os.path.exists(logsDir):
        print("Error: %s doesn't exist" % logsDir)
        return

    for element in os.listdir(logsDir):
        fullName = os.path.join(logsDir, element)
        creationTime = os.path.getctime(fullName)
        currentTime = time.time()
        secondsToExpire = creationTime - currentTime + daysToStoreLogs * 86400  #seconds in 24 hours
        #print("Ctime: %s. Time now: %s\nExpires in %s seconds" % (creationTime, currentTime, secondsToExpire))

        if secondsToExpire < 0:
            print("File or directory %s is too old. Deleting..." % element)

            if os.path.isfile(fullName):
                os.remove(fullName)

            else:
                RemoveDir(fullName)


if __name__ == "__main__":
    DeleteOldLogs(r"C:\Users\beljakov.alexander.a\Desktop\VLSI\TestingFramework\Reports")
