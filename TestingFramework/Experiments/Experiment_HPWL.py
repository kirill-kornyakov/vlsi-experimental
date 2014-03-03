from CoreScripts.CoreFunctions import WriteStringToFile
from CoreScripts.BaseExperiment import BaseExperiment


class Experiment_HPWL(BaseExperiment):
    def __init__(self, logger):
        BaseExperiment.__init__(self, "IWLS05 HPWL experiment", "hpwl_iwls05.cfg", "IWLS05.list",
                                ["HPWL"], ["GP", "LEG", "DP"], logger)

    def CreateEmptyTable(self, reportTable):
        cols = ["Benchmark", "HPWL"]
        WriteStringToFile(cols, reportTable)
