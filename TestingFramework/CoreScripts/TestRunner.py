from ResultsStorage import ResultsStorage
from ExperimentLauncher import ExperimentLauncher
from ExperimentsComparator import ExperimentsComparator
from CoreFunctions import GetTimeStamp, CreateConfigParser
from Logger import Logger
from ParametersParsing import TestRunnerParameters, ReportParameters, GeneralParameters


class TestRunner:
    cfgParser = CreateConfigParser()
    parameters = None
    experiments = []
    storage = ResultsStorage()
    comparator = ExperimentsComparator(storage)

    def __init__(self, parameters=None):
        if parameters is None:
            parameters = TestRunnerParameters(self.cfgParser)

        self.parameters = parameters
        self.experiments = []

    def Append(self, newExperiment):
        self.experiments.append(newExperiment)
        return 0

    def AddExperimentToGroup(self, newExperiment):
        if self.Append(newExperiment) != 0:
            return

        self.comparator.AddExperimentToGroup(newExperiment)

    def Run(self):
        logger = Logger()
        logger.LogStartMessage()
        generalParameters = GeneralParameters(self.cfgParser)
        reportParameters = ReportParameters(self.cfgParser)

        for experiment in self.experiments:
            logger.CoolLog("%s: starting %s" % (GetTimeStamp(), experiment.name))
            launcher = ExperimentLauncher(experiment, self.storage)
            launcher.RunExperiment(generalParameters, reportParameters)

        self.storage.LogResults()

        self.comparator.CompareExperiments(reportParameters)
        logger.CoolLog("Finish")


def test():
    testRunner = TestRunner()
    testRunner.Run()


if __name__ == "__main__":
    test()
