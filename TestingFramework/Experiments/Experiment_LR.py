from CoreScripts.CoreFunctions import WriteStringToFile
from CoreScripts.BaseExperiment import BaseExperiment


class Experiment_LR(BaseExperiment):
    def __init__(self, logger):
        _metrics = ["HPWL", "TWL", "TNS", "WNS"]
        _stages = ["INIT", "LEG"]
        BaseExperiment.__init__(self, "LR experiment IWLS05GP", "LR.cfg", r"IWLS_GP_r1511\IWLS_GP.list", _metrics,
                                _stages, logger)

        self.doParsePQAT = True

    def CreateEmptyTable(self, reportTable):
        cols = ["Benchmark", "initHPWL", "lrHPWL", "dpHPWL",
                "initTWL", "lrTWL", "dpTWL",
                "initTNS", "lrTNS", "dpTNS",
                "initWNS", "lrWNS", "dpWNS", "",
                "lrHPWL%", "dpHPWL%", "lrTWL%", "dpTWL%",
                "lrTNS%", "dpTNS%", "lrWNS%", "dpWNS%"]

        WriteStringToFile(cols, reportTable)

    def AddStringToTable(self, values, benchmark, reportTable):
        cols = [benchmark]

        #print absolute values
        for col in range(len(self.metrics)):
            for row in range(len(self.stages)):
                cols.append(str(values[row][col]))

        cols.append("")

        #print percents
        for col in range(len(self.metrics)):
            for row in range(1, len(self.stages)):
                if values[0][col] != 0.0:
                    percent = 100.0 * (values[row][col] / values[0][col] - 1.0)
                else:
                    percent = -1.0

                printStr = "%.2f;" % percent
                cols.append(printStr)

        WriteStringToFile(cols, reportTable)
