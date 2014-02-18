import os

from CoreScripts import ExperimentLauncher, GeneralParameters, ReportParameters, ResultsStorage, Logger
from CoreScripts.CfgParserFactory import CfgParserFactory
from CoreScripts.Checker import Checker
from CoreScripts.ReportCreator import ReportCreator


class ExperimentRunner:
    nTerminatedBenchmarks = 0
    MAX_TERMINATED_BENCHMARKS_NUM = 3

    def __init__(self):
        self.cfgParser = CfgParserFactory.createCfgParser()
        self.referenceLogs = os.path.join(CfgParserFactory.get_root_dir(),
                                          self.cfgParser.get("ReportParameters", "ReferenceLogs"))

    def parse_benchmarks_list(self, benchmark_list):
        benchmark_list = os.path.join(CfgParserFactory.get_root_dir(),
                                      self.cfgParser.get("GeneralParameters", "benchmarkLists"), benchmark_list)
        benchmarks = ExperimentLauncher.PrepareBenchmarks(benchmark_list)
        
        return benchmarks

    def run_experiment(self, logger, experiment, benchmark, referenceLogFolder):
        checker = Checker(experiment, self.referenceLogs + referenceLogFolder)
        experiment = checker

        generalParameters = GeneralParameters(self.cfgParser)
        reportParameters = ReportParameters(self.cfgParser)

        storage = ResultsStorage()
        launcher = ExperimentLauncher(checker, storage, logger)

        logger.LogD("Config: %s" % experiment.cfg)
        logger.LogD("Benchmarks: %s" % experiment.benchmarks)

        reportCreator = ReportCreator(experiment.name, experiment.cfg, reportParameters)
        logFolder = reportCreator.CreateLogFolder()
        reportTable = reportCreator.GetReportTableName()

        experiment.CreateEmptyTable(reportTable)

        launcher.RunExperimentOnBenchmark(benchmark, logFolder, reportTable, generalParameters)
        logger.LogD("Experiment is finished: %s\n" % benchmark)

        return launcher.experimentResults