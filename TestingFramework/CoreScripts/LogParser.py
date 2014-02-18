from CoreFunctions import CreateConfigParser
from ParametersParsing import LogParserParameters

NOT_FOUND = -1.0

class TableType:
    PFST = "PFST"
    PQAT = "PQAT"


class LogParser:
    logName = ""
    tableHeader = ""
    parameters = None

    def __init__(self, logName, tableType=TableType.PFST, cfgParser=None):
        self.logName = logName

        if cfgParser is None:
            cfgParser = CreateConfigParser()

        self.parameters = LogParserParameters(cfgParser)

        if tableType == TableType.PFST:
            self.tableHeader = self.parameters.PFSTTableHeader
        else:
            self.tableHeader = self.parameters.PQATTableHeader

    def GetFromTable(self, stageTag, metricTag):
        try:
            log = open(self.logName, 'r')
            lines = log.readlines()
            log.close()

        except IOError:
            print("Error: can not open file %s" % self.logName)
            return NOT_FOUND

        #move to the table
        lineIdx = 0
        borderPattern = self.parameters.PFSTBorderPattern

        for line in lines:
            if line.find(self.tableHeader) != -1:
                break
            else:
                lineIdx += 1

        if lineIdx == len(lines):
            print("%s not found in log file" % self.tableHeader)
            return NOT_FOUND

        #find colIdx
        metrics = lines[lineIdx + 1].split()
        colIdx = 0
        for metric in metrics:
            if metrics[colIdx].find(metricTag) == 0:
                break
            else:
                colIdx += 1
        if colIdx == len(metrics):
            print("Tag %s not found" % metricTag)
            return NOT_FOUND

        #parse stages
        lineIdx += 3
        while (lines[lineIdx].find(borderPattern) == -1) and (lines[lineIdx] != "\n"):
            if lines[lineIdx].find(" %s " % stageTag) != -1:
                ll = lines[lineIdx].split()
                return ll[colIdx]
            lineIdx += 1
            if lineIdx == len(lines):
                print("Stage %s not found" % stageTag)
                return NOT_FOUND

        return NOT_FOUND

    def ParsePQAT(self, metrics):
        currStage = 0
        table = []

        while True:
            table.append([])

            for col in range(len(metrics)):
                value = str(self.GetFromTable(str(currStage), metrics[col]))
                value = float(value.replace(",", "."))

                if value == NOT_FOUND:
                    del table[currStage]  #delete empty field
                    return table

                table[currStage].append(value)

            currStage += 1

        return table

    def ParsePFST(self, metrics, stages):
        if stages == []:
            stages.extend(self.GetStageTag())

        # table = [[0 for col in range(len(metrics))] for row in range(len(stages))]
        table = [([0] * len(metrics)) for i in range(len(stages))]

        for col in range(len(metrics)):
            for row in range(len(stages)):
                value = str(self.GetFromTable(stages[row], metrics[col]))

                if value == str(NOT_FOUND):
                    return []

                table[row][col] = float(value.replace(",", "."))

        return table

    def GetStageTag(self):
        log = open(self.logName, 'r')
        lines = log.readlines()
        log.close()
        lineIdx = 0
        linesCount = 0

        for line in lines:
            linesCount += 1

        for line in lines:
            if line.find(self.tableHeader) != -1:
                colIdx = 0
                metrics = lines[lineIdx + 1].split()
                metricTag = "Tag"

                for metric in metrics:
                    if metrics[colIdx].find(metricTag) == 0:
                        break

                    else:
                        colIdx += 1

                if colIdx == len(metrics):
                    print("Tag %s not found" % metricTag)
                    return NOT_FOUND

                lineIdx += 3
                columCount = 0
                i = lineIdx

                while ((lines[i].find(self.parameters.PFSTBorderPattern) == -1)
                       and (lines[i].find("Thats All") == -1) and (i <= linesCount)):
                    columCount += 1
                    i += 1

                tagList = []

                while ((lines[lineIdx].find(self.parameters.PFSTBorderPattern) == -1)
                       and (lines[i].find("Thats All") == -1) and (i <= linesCount)):
                    ll = lines[lineIdx].split()
                    tagList.append(ll[colIdx])
                    lineIdx += 1

                return tagList

            else:
                lineIdx += 1

        if lineIdx == len(lines):
            print("%s not found in log file" % self.tableHeader)
            return NOT_FOUND
