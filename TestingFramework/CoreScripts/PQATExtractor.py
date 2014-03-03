import os

from LogParser import LogParser, PQAT
from CoreFunctions import PrintTableToFile, MakeTableInPercents, ExtractXYFromTable


def ParseAndPlotPQAT(logFolder, doPlotCharts):
    metrics = ["HPWL", "TNS", "WNS"]

    if logFolder == "":
        logFolder = os.getcwd()

    for log in os.listdir(logFolder):
        if os.path.isfile(os.path.join(logFolder, log)) and (".log" == os.path.splitext(log)[-1]):
            logName = os.path.join(logFolder, log)
            parser = LogParser(logName, PQAT)
            table = parser.ParsePQAT(metrics)

            if table == []:
                print("Error: table is empty")
                return

            table = MakeTableInPercents(table)
            PQATFileName = r"%s.csv" % (os.path.basename(log))
            PQATFileName = os.path.join(logFolder, PQATFileName)
            PrintTableToFile(PQATFileName, table, metrics)
            [xValues, yValues] = ExtractXYFromTable(table)

            #Start point (percents)
            xValues[0] = 100
            yValues[0] = 100

def Run():
    #logFolder    = r"../Reports/LR_make_charts"
    #logFolder    = r"../Reports/Relaxation_SGNW"
    #logFolder    = r"../Reports/Relaxation_APlace"
    #logFolder    = r"../Reports/HPWL"

    logFolder = r"../Reports/LR"
    doPlotCharts = True

    if os.path.exists(logFolder) == False:
        print("folder %s does not exist" % logFolder)
        return

    ParseAndPlotPQAT(logFolder, doPlotCharts)
    print("Finished")


if __name__ == "__main__":
    Run()
