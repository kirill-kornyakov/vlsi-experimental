import os

from CoreScripts.BaseExperiment import ComparisonResult, BaseExperiment
from CoreScripts.CoreFunctions import NumberComparisonResult, CompareValues, WriteStringToFile


class Checker(BaseExperiment):
    referenceLogFolder = ""

    def __init__(self, baseExperiment, masterLogFolder):
        BaseExperiment.CopyingConstructor(self, baseExperiment)
        self.referenceLogFolder = masterLogFolder

    def CompareTables(self, table1, table2):
        for col in range(len(self.metrics)):
            for row in range(len(self.stages)):
                compare_result = CompareValues(table1[row][col], table2[row][col])
                if compare_result == NumberComparisonResult.NOT_EQUAL:
                    return ComparisonResult.CHANGED

        return ComparisonResult.OK

    def CreateEmptyTable(self, report_table):
        cols = ["Benchmark"]

        # Write header of a table
        for row in range(len(self.stages)):
            for col in range(len(self.metrics)):
                cols.append("Current %s_%s" % (self.metrics[col], self.stages[row]))
                cols.append("Master %s_%s" % (self.metrics[col], self.stages[row]))

            cols.append("")  # An empty column between metrics on different stages

        WriteStringToFile(cols, report_table)

    def AddStringToTable(self, currentValues, masterValues, benchmark, reportTable):
        cols = [benchmark]

        for row in range(len(self.stages)):
            for col in range(len(self.metrics)):
                cols.append(str(currentValues[row][col]))
                cols.append(str(masterValues[row][col]))

            cols.append("")  #an empty column between metrics on different stages

        #write metrics to the file
        WriteStringToFile(cols, reportTable)

    def read_reference_values(self, logName):
        referenceLogName = os.path.join(self.referenceLogFolder, os.path.basename(logName))
        self.logger.LogD("Parsing: " + referenceLogName)
        referenceValues = self.ParseLog(referenceLogName)

        if referenceValues == []:
            self.logger.Log("Experiment has not failed but master log is empty or does not exist\n")
            return None

        return referenceValues

    def ParseLogAndFillTable(self, logName, benchmark, reportTable, referenceValues):
        self.logger.LogD("Parsing: " + logName)
        currentValues = self.ParseLog(logName)

        if currentValues == []:
            return [ComparisonResult.FAILED, None]

        self.AddStringToTable(currentValues, referenceValues, benchmark, reportTable)

        if self.doParsePQAT:
            self.ParsePQATAndPrintTable(logName)

        return self.CompareTables(currentValues, referenceValues), currentValues
