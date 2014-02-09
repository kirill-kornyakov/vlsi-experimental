import os
import sys
from AcceptanceTest.ExperimentRunner import ExperimentRunner
from CoreScripts import ComparisonResult
from Experiments.Experiment_HippocrateDP import Experiment_HippocrateDP
from Experiments.Experiment_New_Buffering import Experiment_New_Buffering

path = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
sys.path.append(path)

import nose

# TODO:
# "APlace weighting experiment": "IWLS05.list",
# "HPWL_ISPD": "ISPD04.list", -- crash
# "Weighting (SGNW) experiment": "IWLS05.list", -- crash
class TestSanity:
    experimentRunner = ExperimentRunner()

    def run(self, experiment, benchmark, referenceLogFolder):
        result = self.experimentRunner.run_experiment(experiment, benchmark, referenceLogFolder)
        assert result == ComparisonResult.OK

    def test_hippocrate(self):
        benchmark_list = "sanity/HippocrateDP.list"
        referenceLogFolder = "/HippocrateDP"
        experiment = Experiment_HippocrateDP()

        benchmarks = self.experimentRunner.parse_benchmarks_list(benchmark_list)
        for benchmark in benchmarks:
            yield self.run, experiment, benchmark, referenceLogFolder

    def test_new_buffering(self):
        benchmark_list = "sanity/New_Buffering.list"
        referenceLogFolder = "/New_Buffering/IWLS"
        experiment = Experiment_New_Buffering()

        benchmarks = self.experimentRunner.parse_benchmarks_list(benchmark_list)
        for benchmark in benchmarks:
            yield self.run, experiment, benchmark, referenceLogFolder

    # def DISABLED_test_HPWL_placement(self):
    #     benchmark_list = "IWLS05.list"
    #     referenceLogFolder = "/HPWL/IWLS"
    #     experiment = Experiment_HPWL()
    #     self.experimentRunner.run_experiment(experiment, benchmark_list, referenceLogFolder)
    #
    # def DISABLED_test_lr(self):
    #     benchmark_list = "IWLS_GP_r1511_fast.list"
    #     referenceLogFolder = "/LR"
    #     experiment = Experiment_LR()
    #     self.experimentRunner.run_experiment(experiment, benchmark_list, referenceLogFolder)

if __name__ == "__main__":
    nose.main()