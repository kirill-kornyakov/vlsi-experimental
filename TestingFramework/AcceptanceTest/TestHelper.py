from AcceptanceTest.ExperimentRunner import ExperimentRunner
from CoreScripts import ComparisonResult


class TestHelper:
    experimentRunner = ExperimentRunner()

    def print_differences(self, experiment_results, benchmark):
        ref_pfst = experiment_results.pfstRefTables.get(benchmark)
        assert ref_pfst != None
        curr_pfst = experiment_results.pfstTables.get(benchmark)
        assert curr_pfst != None
        for x in zip(ref_pfst, curr_pfst):
            print x[0]
            print x[1]
            print [round(b / a, 2) for a, b in zip(x[0], x[1])]
            print ""

    def run(self, logger, experiment, benchmark, referenceLogFolder):
        experiment_results = self.experimentRunner.run_experiment(logger, experiment, benchmark, referenceLogFolder)
        result = experiment_results.benchmarkResults.get(benchmark)
        if result != ComparisonResult.OK:
            self.print_differences(experiment_results, benchmark)
            assert result == ComparisonResult.OK

    def expand_benchmark_list(self, benchmark_list):
        return self.experimentRunner.parse_benchmarks_list(benchmark_list)