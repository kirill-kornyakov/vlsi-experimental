from CoreScripts.CoreFunctions import WriteStringToFile
from CoreScripts.BaseExperiment import BaseExperiment


class Experiment_HippocrateDP(BaseExperiment):
    def __init__(self, logger):
        _metrics = ["HPWL", "TNS", "WNS"]
        _stages = ["LEG", "DP"]
        BaseExperiment.__init__(self, "HippocrateDP experiment",
                                "HippocrateDP.cfg", "IWLS_GP_Hippocrate.list", _metrics, _stages, logger)

    def CreateEmptyTable(self, reportTable):
        cols = ["", "GP + Leg", "", "", "", "GP + Leg + HippocrateDP"]
        WriteStringToFile(cols, reportTable)

        cols = ["Benchmark", "HPWL", "TNS", "WNS", "", "HPWL", "TNS", "WNS"]
        WriteStringToFile(cols, reportTable)
