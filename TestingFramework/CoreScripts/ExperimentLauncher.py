import subprocess

from BaseExperiment import *
from ReportCreator import ReportCreator
from CoreFunctions import ReportErrorAndExit


TERMINATED = "Terminated"
MAX_TERMINATED_BENCHMARKS_NUM = 3


class ExperimentLauncher:
    logger = None
    experiment = None
    resultsStorage = None
    experimentResults = None
    nTerminatedBenchmarks = 0

    def __init__(self, experiment, resultsStorage, logger):
        self.logger = logger
        self.experiment = experiment
        self.resultsStorage = resultsStorage
        self.experimentResults = ExperimentResults()
        self.nTerminatedBenchmarks = 0

    def AddErrorToResults(self, error):
        self.logger.Log(error)
        self.experimentResults.AddError(error)

    def CheckParameters(self):
        #check if config file can be found
        if not os.path.exists(self.experiment.cfg):
            error = "Error: file '%s' not found" % self.experiment.cfg
            self.AddErrorToResults(error)
            return False

        return True

    @staticmethod
    def PrepareBenchmarks(benchmark_list):
        #check if benchmarks list file can be found
        if not os.path.exists(benchmark_list):
            error = "Error: file '%s' not found" % benchmark_list
            # self.AddErrorToResults(error)
            return None

        notFoundBenchmarksStr = ""
        notFoundBenchmarks = []
        benchmarks = (open(benchmark_list).read()).split("\n")

        # Perform filtering of empty lines and commented by # benchmarks
        benchmarks = [x for x in benchmarks if not x.strip().startswith('#')]
        benchmarks = [x for x in benchmarks if len(x.strip())]

        cfgParser = CfgParserFactory.createCfgParser()
        benchmarkFolder = os.path.join(CfgParserFactory.get_root_dir(),
                                       cfgParser.get("GeneralParameters", "benchmarkCheckoutPath"))

        # Check if all benchmarks can be found
        for i in range(len(benchmarks)):
            benchmarks[i] = benchmarks[i].strip()
            benchmark = r"%s.def" % (benchmarks[i])
            # benchmarkFolder = os.path.dirname(os.path.abspath(benchmark_list))
            benchmark = os.path.join(benchmarkFolder, benchmark)

            if not os.path.exists(benchmark):
                notFoundBenchmarks.append(benchmarks[i])

        # Print and delete from list benchmarks which were not found
        if notFoundBenchmarks != []:
            for benchmark in notFoundBenchmarks:
                benchmarks.remove(benchmark)
                notFoundBenchmarksStr += ' "%s";' % benchmark

            error = "Error: benchmarks %s were not found!" % notFoundBenchmarksStr
            # self.AddErrorToResults(error)

        if benchmarks == []:
            return None

        return benchmarks

    def CheckParametersAndPrepareBenchmarks(self):
        if not self.CheckParameters():
            return []

        return self.PrepareBenchmarks()

    def RunExperiment(self, generalParameters, reportParameters):
        self.logger.Log("Config: %s" % self.experiment.cfg)
        self.logger.Log("Benchmarks: %s" % self.experiment.benchmarks)

        reportCreator = ReportCreator(self.experiment.name, self.experiment.cfg, reportParameters)
        logFolder = reportCreator.CreateLogFolder()
        reportTable = reportCreator.GetReportTableName()

        self.experiment.CreateEmptyTable(reportTable)
        benchmarks = self.CheckParametersAndPrepareBenchmarks()

        if benchmarks == []:
            self.resultsStorage.AddExperimentResult(self.experiment.name, self.experimentResults)
            return

        for benchmark in benchmarks:
            if self.nTerminatedBenchmarks >= MAX_TERMINATED_BENCHMARKS_NUM:
                self.AddErrorToResults("Reached maximum number of terminated benchmarks")
                break

            self.RunExperimentOnBenchmark(benchmark, logFolder, reportTable, generalParameters)
            self.logger.Log("[%s/%s] %s is finished\n" % (benchmarks.index(benchmark) + 1, len(benchmarks),
                                                          benchmark))

        self.resultsStorage.AddExperimentResult(self.experiment.name, self.experimentResults)

    def PreparePlacerParameters(self, benchmark, logFolder, generalParameters):
        self.experimentResults.AddPFSTForBenchmark(benchmark, [])

        defFile = r"%s.def" % benchmark
        cfgParser = CfgParserFactory.createCfgParser()
        benchmarkFolder = os.path.join(CfgParserFactory.get_root_dir(),
                                       cfgParser.get("GeneralParameters", "benchmarkCheckoutPath"))
        # benchMarkFolder = os.path.dirname(os.path.abspath(benchmark))
        defFile = os.path.join(benchmarkFolder, defFile)
        defFileParam = "--params.def=%s" % defFile

        pixFolder = os.path.abspath(os.path.join(logFolder, os.path.basename(benchmark), "pix"))
        pixDirParam = "--plotter.pixDirectory=%s" % pixFolder

        milestonePixFolder = os.path.abspath(os.path.join(pixFolder, "milestones"))

        if os.path.exists(milestonePixFolder) != True:
            os.makedirs(milestonePixFolder)

        milestonePixDirParam = "--plotter.milestonePixDirectory=%s" % milestonePixFolder

        exeName = os.path.join(generalParameters.binDir, "itlPlace")  #FIXME: introduce variable
        placerParameters = [exeName, os.path.abspath(self.experiment.cfg), defFileParam, pixDirParam,
                            milestonePixDirParam]
        placerParameters.extend(self.experiment.cmdArgs)

        #HACK: ugly hack for ISPD04 benchmarks
        if self.experiment.cfg.find("ispd04") != -1:
            lefFile = r"%s.lef" % benchmark
            lefFile = os.path.join(benchmarkFolder, lefFile)
            lefFileParam = "--params.lef=%s" % lefFile
            placerParameters.append(lefFileParam)

        logFileName = r"%s.log" % (os.path.basename(benchmark))
        logFileName = os.path.join(logFolder, logFileName)
        return placerParameters, logFileName

    def RunPlacer(self, placerParameters, logFileName, generalParameters):
        fPlacerOutput = open(logFileName, 'w')
        self.logger.LogD("Output will be redirected to: " + logFileName)
        self.logger.LogD("Working directory is set to: " + generalParameters.binDir)

        try:  # FIXME: this place is debugged now; may be use placerParameters directly
            #p = subprocess.Popen(placerParameters, stdout = fPlacerOutput, cwd = generalParameters.binDir)
            params = placerParameters
            params[0] = "./" + os.path.basename(params[0])
            p = subprocess.Popen(params, cwd=generalParameters.binDir, stdout=fPlacerOutput)
            self.logger.LogD("Executed the following command:\n" + " ".join(params) + "\n")

        except Exception, e:
            error = "Error: can not call %s \n" % (placerParameters[0])
            error = error + "Exception message: " + str(e)
            ReportErrorAndExit(error, self.logger)

        #FIXME: here we wait for placer to finish, but we need a cross-platform way to do this
        p.wait()
        # t_start = time.time()
        # seconds_passed = 0
        # wasOpened = False
        # time.sleep(1)
        # while(wasOpened == False and seconds_passed < generalParameters.maxTimeForBenchmark):
        #   seconds_passed = time.time() - t_start

        #   try:
        #     wasOpened = open(placerParameters[0], 'a')
        #     wasOpened.close()

        #   except IOError:
        #     wasOpened = False
        # self.logger.Log("Seconds passed: %.2f" % (seconds_passed))

        placerReturnCode = p.poll()
        if placerReturnCode is None:
            try:
                p.terminate()

            except Exception:
                pass

        fPlacerOutput.close()
        return placerReturnCode

    def SaveResults(self, placerReturnCode, logFileName, benchmark, reportTable):
        reference_values = self.experiment.read_reference_values(logFileName)
        self.experimentResults.AddReferencePFSTForBenchmark(benchmark, reference_values)

        (result, pfst_values) = self.experiment.ParseLogAndFillTable(logFileName, benchmark, reportTable, reference_values)

        self.experimentResults.AddPFSTForBenchmark(benchmark, pfst_values)
        self.experimentResults.AddBenchmarkResult(benchmark, result)

        if result == ComparisonResult.CHANGED:
            self.experimentResults.resultFile = reportTable

        if placerReturnCode != 0:
            self.logger.Log("Process return code: %s" % placerReturnCode)
            return
        elif placerReturnCode is None:
            self.logger.Log("Time out on %s" % benchmark)
            self.nTerminatedBenchmarks += 1
            self.experimentResults.AddBenchmarkResult(benchmark, TERMINATED)
            return

    def RunExperimentOnBenchmark(self, benchmark, logFolder, reportTable, generalParameters):
        placerParameters, logFileName = self.PreparePlacerParameters(benchmark, logFolder, generalParameters)
        placerReturnCode = self.RunPlacer(placerParameters, logFileName, generalParameters)
        self.SaveResults(placerReturnCode, logFileName, benchmark, reportTable)
