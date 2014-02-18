from CoreScripts.TestRunner import TestRunner
from CoreScripts.BaseExperiment import BaseExperiment


class Experiment_New_Buffering(BaseExperiment):
    def __init__(self, logger):
        _metrics = ['Cells', 'HPWL', 'TWL', 'TNS', 'WNS']
        _stages = ['INIT', 'NBUF', 'LEGB']

        BaseExperiment.__init__(self, 'IWLS05 new_buffering experiment (NBL)', 'NewBuffering.cfg', 'IWLS05.list',
                                _metrics, _stages, logger)


def TestRun():
    metrics = ['Time', 'Cells', 'HPWL', 'TWL', 'TNS', 'WNS']
    stages = ['INIT', 'NBUF', 'LEGB']

    TotalAllowableBuffersAreas = ['0.001', '0.005', '0.01', '0.03']
    Intervals = ['2', '20']
    AdaptiveSizeBufferMultipliers = ['false', 'true']
    NumberMetaIterationStartBufferings = ['1', '2']

    testRunner = TestRunner()
    for NumberMetaIterationStartBuffering in NumberMetaIterationStartBufferings:
        for AdaptiveSizeBufferMultiplier in AdaptiveSizeBufferMultipliers:
            for Interval in Intervals:
                for TotalAllowableBuffersArea in TotalAllowableBuffersAreas:
                    expName = 'NBL_int_' + Interval + '_Area_' + TotalAllowableBuffersArea + '_AdaptiveMultipliers_' + AdaptiveSizeBufferMultiplier + '_NumberMetaIteration_' + NumberMetaIterationStartBuffering
                    print(expName)
                    cmdArgs = ["--plotter.pixDirectory=.\\\\pix\\\\" + expName + "\\\\",
                               " --LSE.GlobalPlacement.New_Buffering.AdaptiveSizeBufferMultiplier=" + AdaptiveSizeBufferMultiplier,
                               " --LSE.GlobalPlacement.New_Buffering.TotalAllowableBuffersArea=" + TotalAllowableBuffersArea,
                               " --LSE.GlobalPlacement.New_Buffering.NumberMetaIterationStartBuffering=" + NumberMetaIterationStartBuffering,
                               " --LSE.GlobalPlacement.New_Buffering.Interval=" + Interval]
                    print(cmdArgs)

                    e = BaseExperiment(expName, 'New_buffering.cfg', 'IWLS05.list', metrics, stages, cmdArgs)
                    testRunner.AddExperimentToGroup(e)
    testRunner.Run()

if __name__ == "__main__":
    TestRun()