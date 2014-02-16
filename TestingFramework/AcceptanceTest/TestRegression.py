import os
import sys
from CoreScripts import Logger
from Experiments.Experiment_HPWL import Experiment_HPWL
from Experiments.Experiment_LR import Experiment_LR
from Experiments.Experiment_Weighting import Experiment_Weighting

path = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
sys.path.append(path)

import nose
from nose.tools import nottest

from AcceptanceTest.TestHelper import TestHelper
from Experiments.Experiment_HippocrateDP import Experiment_HippocrateDP
from Experiments.Experiment_New_Buffering import Experiment_New_Buffering


class TestRegression:
    test_helper = TestHelper()
    logger = Logger()

    @nottest
    def test_hippocrate(self):
        benchmark_list = "sanity/HippocrateDP.list"
        referenceLogFolder = "/HippocrateDP"
        experiment = Experiment_HippocrateDP(self.logger)

        benchmarks = self.test_helper.expand_benchmark_list(benchmark_list)
        for benchmark in benchmarks:
            yield self.test_helper.run, self.logger, experiment, benchmark, referenceLogFolder

    @nottest
    def test_hpwl_placement(self):
        benchmark_list = "regression/HPWL_IWLS.list"
        referenceLogFolder = "/HPWL/IWLS-initial-mac-state"
        experiment = Experiment_HPWL(self.logger)

        benchmarks = self.test_helper.expand_benchmark_list(benchmark_list)
        for benchmark in benchmarks:
            yield self.test_helper.run, self.logger, experiment, benchmark, referenceLogFolder

    @nottest
    def test_new_buffering(self):
        benchmark_list = "regression/New_Buffering.list"
        referenceLogFolder = "/New_Buffering/IWLS-initial-mac-state"
        experiment = Experiment_New_Buffering(self.logger)

        benchmarks = self.test_helper.expand_benchmark_list(benchmark_list)
        for benchmark in benchmarks:
            yield self.test_helper.run, self.logger, experiment, benchmark, referenceLogFolder

    @nottest
    def test_lr(self):
        benchmark_list = "regression/LR.list"
        referenceLogFolder = "/LR-initial-mac-state"
        experiment = Experiment_LR(self.logger)

        benchmarks = self.test_helper.expand_benchmark_list(benchmark_list)
        for benchmark in benchmarks:
            yield self.test_helper.run, self.logger, experiment, benchmark, referenceLogFolder

    @nottest
    def test_hpwl_ispd04(self):
        benchmark_list = "regression/ISPD04.list"
        experiment = Experiment_HPWL(self.logger)
        experiment.SetConfig("hpwl_ispd04.cfg")
        experiment.name = "HPWL ISPD04"
        referenceLogFolder = "/HPWL/ISPD-initial-mac-state"

        benchmarks = self.test_helper.expand_benchmark_list(benchmark_list)
        for benchmark in benchmarks:
            yield self.test_helper.run, self.logger, experiment, benchmark, referenceLogFolder

    ## APlace Weighting, "IWLS05.list"
    # exp_W.name = "APlace weighting experiment"
    # exp_W.SetConfig("APlace_weighting.cfg")
    # chk_APW = Checker(exp_W, referenceLogs + r"/Weighting/SensitivityGuided")
    # @nottest
    def test_aplace_weighting(self):
        benchmark_list = "regression/APlace_Weighting.list"
        experiment = Experiment_Weighting(self.logger)
        experiment.SetConfig("APlace_weighting.cfg")
        experiment.name = "APlace Weighting"
        referenceLogFolder = "/Weighting/APlace"

        benchmarks = self.test_helper.expand_benchmark_list(benchmark_list)
        for benchmark in benchmarks:
            yield self.test_helper.run, self.logger, experiment, benchmark, referenceLogFolder

    # TODO: enable tests below
    ## Sensitivity-Guided Weighting, "IWLS05.list" -- crash
    # exp_W = Experiment_Weighting()
    # chk_SGW = Checker(exp_W, referenceLogs + r"/Weighting/SensitivityGuided")


if __name__ == "__main__":
    nose.main()