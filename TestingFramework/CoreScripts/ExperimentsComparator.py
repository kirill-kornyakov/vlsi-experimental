from CoreFunctions import GetTimeStamp, WriteStringToFile, CompareValues

from ReportCreator import ReportCreator
from Logger import Logger


class ExperimentsComparator:
    logger = None
    storage = None
    masterExperiment = None
    masterResuts = {}  #benchmark: pfst
    experimentsToCompare = {}  #Group of experiments. Their results will be compared
    #experimentName: {benchmark: pfst}

    def __init__(self, storage):
        self.storage = storage
        self.masterExperiment = None
        self.masterResuts = {}
        self.experimentsToCompare = {}

    def AddExperimentToGroup(self, newExperiment):
        if self.masterExperiment is None:
            self.masterExperiment = newExperiment

        else:
            if newExperiment.benchmarks != self.masterExperiment.benchmarks:
                self.logger.Log("Error: list files are not equal!")
                exit(1)

            if newExperiment.metrics != self.masterExperiment.metrics:
                self.logger.Log("Error: metrics are not equal!")
                exit(1)

            self.experimentsToCompare[newExperiment.name] = {}

    def GetResultsForExperiment(self, experimentName):
        if experimentName in self.storage.experimentResults:
            return self.storage.experimentResults[experimentName].pfstTables

        self.logger.Log("Error: results for %s not found" % experimentName)
        return {}

    def GetExperimentsResults(self):
        #Masater experiment
        self.masterResuts = self.GetResultsForExperiment(self.masterExperiment.name)

        #Normal experiments
        for experimentName in self.experimentsToCompare.iterkeys():
            self.experimentsToCompare[experimentName] = self.GetResultsForExperiment(experimentName)

    @staticmethod
    def PrintLabelAndBlanks(cols, label, nMetrics):
        cols.extend(["", label])

        #FIXME: bug, unused `col` variable
        for col in nMetrics:
            cols.append("")

    def PrintTableHeader(self, resultFileName):
        metrics = self.masterExperiment.metrics
        nMetrics = range(len(metrics))

        # First line of the table header
        cols = []

        self.PrintLabelAndBlanks(cols, "INIT", nMetrics)
        self.PrintLabelAndBlanks(cols, self.masterExperiment.name, nMetrics)

        for experimentName in self.experimentsToCompare.iterkeys():
            self.PrintLabelAndBlanks(cols, experimentName, nMetrics)

        WriteStringToFile(cols, resultFileName)

        # Second line of the table header
        cols = ["benchmark"]

        #Init values
        """for row in nMetrics:
          cols.append(metrics[row])"""
        cols.extend(metrics)
        cols.append("")  #an empty column between INIT and experiments'es values

        #Target experiment's values
        ##for row in nMetrics: #TODO: check
        ##  cols.append(metrics[row])
        cols.extend(metrics)
        cols.append("")

        nNormalExperiments = len(self.experimentsToCompare)

        for experimentName in xrange(nNormalExperiments):
            for row in nMetrics:
                if metrics[row] != "Time":
                    cols.append(metrics[row] + "%")

                else:
                    cols.append(metrics[row])

            cols.append("")  #an empty column between experiments'es results

        WriteStringToFile(cols, resultFileName)

    def MakeResultTable(self, resultFileName):
        self.PrintTableHeader(resultFileName)
        benchmarks = list(self.masterResuts.keys())

        for benchmark in benchmarks:
            newTableLine = self.CreateNewTableLine(benchmark)
            WriteStringToFile(newTableLine, resultFileName)

    def CreateNewTableLine(self, benchmark):
        metrics = self.masterExperiment.metrics
        nMetrics = range(len(metrics))
        bestValues = [1000000 for i in nMetrics]
        bestValuesIdx = [0 for i in nMetrics]
        experimentsLine = []

        # INIT metrics
        initialMetrics = []
        initialMetricsLine = []

        # reference metrics
        referenceMetrics = []
        resultValues = self.masterResuts[benchmark]

        if resultValues != []:  #If target experiment succeed on current benchmark
            for metricIdx in nMetrics:
                #Fill reference metrics
                finalStageIdx = len(self.masterExperiment.stages) - 1
                value = resultValues[finalStageIdx][metricIdx]
                experimentsLine.append(str(value))
                referenceMetrics.append(value)

                #Fill Initial metrics
                value = resultValues[0][metricIdx]
                initialMetricsLine.append(str(value))
                initialMetrics.append(value)

            initialMetricsLine.append("")

        else:  #Target experiment failed
            referenceMetrics = initialMetrics

            for metricIdx in nMetrics:
                experimentsLine.append("N/A")

        experimentsLine.append("")

        # normal experiments
        for experimentName, experimentResults in self.experimentsToCompare.iteritems():
            resultValues = experimentResults[benchmark]

            if resultValues == []:
                for metricIdx in nMetrics:
                    experimentsLine.append("N/A")

                experimentsLine.append("")
                continue

            #if 'INITIAL' metrics haven't been printed yet
            if initialMetrics == []:
                #take them from the table of this experiment
                for metricIdx in nMetrics:
                    value = resultValues[0][metricIdx]
                    initialMetricsLine.append(str(value))
                    initialMetrics.append(value)

                initialMetricsLine.append("")

            #else compare 'INITIAL' metrics
            else:
                for metricIdx in nMetrics:
                    cmpResult = CompareValues(initialMetrics[metricIdx], resultValues[0][metricIdx])

                    if cmpResult == NOT_EQUAL and metrics[metricIdx] != "Time":
                        self.logger.Log("Error: not equal INIT metrics")

            finalStageIdx = - 1  #Index of the last elemnt in the list

            for metricIdx in nMetrics:
                valueStr = resultValues[finalStageIdx][metricIdx]

                if metrics[metricIdx] != "Time":
                    percent = 100 * valueStr / referenceMetrics[metricIdx]
                    valueStr = "%.2f" % percent

                if percent < bestValues[metricIdx]:
                    bestValues[metricIdx] = percent  #remember the best result
                    bestValuesIdx[metricIdx] = len(experimentsLine)  #and its index

                experimentsLine.append(valueStr)

            experimentsLine.append("")

        #for idx in bestValuesIdx:
        #  if (idx > 0):
        #    experimentsLine[idx] = MarkResultAsBest(experimentsLine[idx])

        if initialMetricsLine == []:
            for metricIdx in nMetrics:
                initialMetricsLine.append("N/A")

            initialMetricsLine.append("")

        newTableLine = [benchmark]
        newTableLine.extend(initialMetricsLine)
        newTableLine.extend(experimentsLine)
        return newTableLine

    def CompareExperiments(self, reportParameters):
        if len(self.experimentsToCompare) > 0:
            self.logger = Logger()
            self.logger.CoolLog("Comparing experiments")

            try:
                reportCreator = ReportCreator("Comparing", "Comparing %s" % (GetTimeStamp()), reportParameters)
                logFolder = reportCreator.CreateLogFolder()
                cmpFileName = reportCreator.GetReportTableName()
                self.GetExperimentsResults()
                self.MakeResultTable(cmpFileName)

            except:
                import traceback

                self.logger.Log("exception: %s" % (traceback.format_exc()))

            self.logger.Log("Comparing finished")
