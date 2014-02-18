from CoreScripts.TestRunner import TestRunner
from CoreScripts.BaseExperiment import BaseExperiment

#Sensitivity Guided Net Weighting experiment
class Experiment_Weighting(BaseExperiment):
    def __init__(self, logger):
        _metrics = ["HPWL", "TNS", "WNS"]
        _stages = ["INIT", "LEG"]
        BaseExperiment.__init__(self, "Weighting (SGNW) experiment",
                                "Sensitivity_guided_weighting.cfg", "IWLS05.list",
                                _metrics, _stages, logger)

        self.doParsePQAT = True


def test():
    testRunner = TestRunner()
    eSGNW = Experiment_Weighting()
    eSGNW.SetBenchmarksList("IWLS05_fast.list")
    #testRunner.Append(eSGNW)

    metrics = ["HPWL", "TNS", "WNS"]
    #stages  = ["INIT", "LEG1", "LEG2", "LEG3", "LEG4", "LEG5", "LEG6", "LEG7", "LEG8", "LEG9", "LEG"]
    stages = ["INIT", "LEG"]

    cValues = [600]
    expName = "Weighting (SGNW) experiment_C_"
    cmdArgs = "--NetWeighting.SensitivityGuidedNetWeighting.C="

    for cValue in cValues:
        #cmdArgs = "--NetWeighting.SensitivityGuidedNetWeighting.C=1000"

        e = BaseExperiment("%s%s" % (expName, str(cValue)), "Sensitivity_guided_weighting.cfg", "IWLS05_fast.list",
                           metrics, stages, [cmdArgs + str(cValue)])
        testRunner.AddExperimentToGroup(e)

    eLeg = BaseExperiment("Leg1", "Sensitivity_guided_weighting.cfg", "IWLS05_fast.list", metrics,
                          ["INIT", "LEG1"], [])  #["--DesignFlow.LoopGlobalPlacement=false")]
    testRunner.AddExperimentToGroup(eLeg)
    testRunner.Run()


def test2():
    testRunner = TestRunner()
    eSGNW = Experiment_Weighting()
    testRunner.Append(eSGNW)
    testRunner.Run()


if __name__ == "__main__":
    test2()
