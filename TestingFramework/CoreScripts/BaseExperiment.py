import os

from CoreScripts.CfgParserFactory import CfgParserFactory
from ParametersParsing import GeneralParameters
from LogParser import LogParser, TableType
from CoreFunctions import CreateConfigParser, WriteStringToFile, MakeTableInPercents, PrintTableToFile


class ComparisonResult:
    OK = "OK"
    NEW = "New"
    FAILED = "Failed"
    CHANGED = "Changed"

class ExperimentResults:
    errors = []
    resultFile = ""  #file with result table
    pfstTables = {}  #benchmark: pfst
    pfstRefTables = {}  #benchmark: pfst
    benchmarkResults = {}  #result: [benchmarks]

    def __init__(self):
        self.errors = []
        self.resultFile = ""
        self.pfstTables = {}
        self.pfstRefTables = {}
        self.benchmarkResults = {}

    def GetPFSTForBenchmark(self, benchmark):
        return self.pfstTables[benchmark]

    def GetReferencePFSTForBenchmark(self, benchmark):
        return self.pfstRefTables[benchmark]

    def AddError(self, error):
        self.errors.append(error)

    def AddBenchmarkResult(self, benchmark, result):
        if not benchmark in self.benchmarkResults.keys():
            self.benchmarkResults[benchmark] = []

        self.benchmarkResults[benchmark] = result

    def AddPFSTForBenchmark(self, benchmark, table):
        self.pfstTables[benchmark] = table

    def AddReferencePFSTForBenchmark(self, benchmark, table):
        self.pfstRefTables[benchmark] = table

    def __str__(self):
        resultStr = ""

        for (benchmark, result) in list(self.benchmarkResults.iteritems()):
            resultStr += ("%s: %s\n" % (result, benchmark))

        for error in self.errors:
            resultStr += ("%s\n" % error)

        return resultStr + "\n"

    def Print(self):
        print(self.__str__())


class BaseExperiment:
    name = ""
    cfg = ""
    benchmarks = ""
    cmdArgs = []  #list of command line arguments
    metrics = []
    stages = []
    doParsePQAT = False

    cfgParser = CreateConfigParser()
    generalParameters = GeneralParameters(cfgParser)

    def __init__(self, name, cfg, benchmarks, metrics, stages, logger, cmdArgs=None):
        if not cmdArgs: cmdArgs = []
        self.name = name
        self.cfg = os.path.join(self.generalParameters.binDir, "cfg", cfg)
        self.benchmarks = self.SetBenchmarksList(benchmarks)
        self.cmdArgs = cmdArgs
        self.metrics = metrics
        self.stages = stages
        self.logger = logger

    def CopyingConstructor(self, be):
        self.cfg = be.cfg
        self.name = be.name
        self.stages = be.stages
        self.metrics = be.metrics
        self.cmdArgs = be.cmdArgs
        self.benchmarks = be.benchmarks
        self.doParsePQAT = be.doParsePQAT
        self.logger = be.logger

    def SetConfig(self, cfg):
        self.cfg = os.path.join(self.generalParameters.binDir, "cfg", cfg)

    def SetBenchmarksList(self, benchmarks):
        self.benchmarks = os.path.join(CfgParserFactory.get_root_dir(), self.cfgParser.get("GeneralParameters", "benchmarkLists"), benchmarks)

    def CreateEmptyTable(self, reportTable):
        cols = ["Benchmark"]

        #write header of a table.
        for row in range(len(self.stages)):
            for col in range(len(self.metrics)):
                cols.append("%s_%s" % (self.metrics[col], self.stages[row]))

            cols.append("")  #an empty column between metrics on different stages

        WriteStringToFile(cols, reportTable)

    def ParseLog(self, logName):
        parser = LogParser(logName, TableType.PFST, self.cfgParser)
        return parser.ParsePFST(self.metrics, self.stages)

    def ParsePQATAndPrintTable(self, logName):
        metrics = ["HPWL", "TNS", "WNS"]
        parser = LogParser(logName, TableType.PQAT, self.cfgParser)
        table = parser.ParsePQAT(metrics)
        table = MakeTableInPercents(table)
        pqatName = r"%s.csv" % (os.path.basename(logName))
        PQATFileName = os.path.join(os.path.dirname(logName), pqatName)
        PrintTableToFile(PQATFileName, table, metrics)

    def AddStringToTable(self, values, benchmark, reportTable):
        cols = [benchmark]

        for row in range(len(self.stages)):
            for col in range(len(self.metrics)):
                cols.append(str(values[row][col]))

            cols.append("")  #an empty column between metrics on different stages

        #write metrics to the file
        WriteStringToFile(cols, reportTable)

    def MakeResultTable(self, logFolder, reportTable):
        if os.path.exists(logFolder) == False:
            # logger = Logger()
            self.logger.Log("Error: folder %s does not exist" % logFolder)
            return

        reportTable = os.path.join(logFolder, reportTable)
        self.CreateEmptyTable(reportTable)

        for log in os.listdir(logFolder):
            if os.path.isfile(os.path.join(logFolder, log)) and (".log" == os.path.splitext(log)[-1]):
                benchmark = os.path.splitext(log)[0]
                self.ParseLogAndFillTable(os.path.join(logFolder, log), benchmark, reportTable)

    def ParseLogAndFillTable(self, logName, benchmark, reportTable):
        values = self.ParseLog(logName)

        if values == []:
            return [ComparisonResult.FAILED, []]

        self.AddStringToTable(values, benchmark, reportTable)

        if self.doParsePQAT == True:
            self.ParsePQATAndPrintTable(logName)

        return [ComparisonResult.OK, values]


def TestResultTableMaking():
    stages = ["LEG", "DP"]
    metrics = ["HPWL", "TNS", "WNS"]
    experiment = BaseExperiment("HippocrateDP experiment", "HippocrateDP.cfg", "IWLS_GP_Hippocrate.list",
                                metrics, stages)

    experiment.MakeResultTable(r"..\Reports\HippocrateDP", "TestTable2.csv")


def test():
    from TestRunner import TestRunner

    stages = ["LEG", "DP"]
    metrics = ["HPWL", "TNS", "WNS"]
    experiment = BaseExperiment("HippocrateDP experiment", "HippocrateDP.cfg", "IWLS_GP_Hippocrate.list",
                                metrics, stages)

    testRunner = TestRunner()
    testRunner.Append(experiment)
    testRunner.Run()


if __name__ == "__main__":
    test()