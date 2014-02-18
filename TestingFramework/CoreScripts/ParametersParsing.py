import os
import traceback
from ConfigParser import ConfigParser

from CoreScripts.CfgParserFactory import CfgParserFactory


class TestRunnerParameters:
    doCheckout = False
    doBuild = False

    def __init__(self, cfgParser):
        doBuild = cfgParser.get("TestRunnerParameters", "doBuild")
        doCheckout = cfgParser.get("TestRunnerParameters", "doCheckout")

        if doBuild == "True":
            self.doBuild = True

        if doCheckout == "True":
            self.doCheckout = True

    def PrintParameters(self):
        print("TestRunner parameters:\ndoCheckout = %s\ndoBuild = %s" % (self.doCheckout, self.doBuild))


class EmailerParameters:
    doSendMail = False
    subject = "UnitTest"
    recipients = None
    sender = "itlab.vlsi@yandex.ru"
    smtpserver = "smtp.yandex.com"
    smtpuser = "itlab.vlsi@yandex.ru"  # for SMTP AUTH, set SMTP username here
    smtppass = "itlPlace"

    def __init__(self, cfgParser=None):
        if cfgParser is not None:
            doSendMail = cfgParser.get("EmailerParameters", "doSendMail")
            self.subject = cfgParser.get("EmailerParameters", "subject")
            self.sender = cfgParser.get("EmailerParameters", "sender")
            self.smtpserver = cfgParser.get("EmailerParameters", "smtpserver")
            self.smtpuser = cfgParser.get("EmailerParameters", "smtpuser")
            self.smtppass = cfgParser.get("EmailerParameters", "smtppass")
            recipientsStr = cfgParser.get("EmailerParameters", "recipients")

            self.recipients = recipientsStr.split(",")

            if doSendMail == "True":
                self.doSendMail = True

    def PrintParameters(self):
        print("Emailer parameters:\ndoSendMail = %s\nsubject = %s\nrecipients = %s\nsender = %s\n\
           smtpserver = %s\nsmtpuser = %s\nsmtppass = %s" % (self.doSendMail, self.subject,
                                                             str(self.recipients), self.sender, self.smtpserver,
                                                             self.smtpuser, self.smtppass))


class RepoParameters:
    repoPath = r"http://svn.software.unn.ru/VLSI/CODE/trunk"
    srcRepoPath = repoPath + r"/itlPlace/"
    benchRepoPath = repoPath + r"/Benchmarks/"

    def __init__(self, cfgParser):
        self.repoPath = r"%s" % (cfgParser.get("RepoParameters", "repoPath"))
        self.srcRepoPath = r"%s" % (cfgParser.get("RepoParameters", "srcRepoPath"))
        self.benchRepoPath = r"%s" % (cfgParser.get("RepoParameters", "benchRepoPath"))

    def PrintParameters(self):
        print("Repo parameters:\nrepoPath = %s\nsrcRepoPath = %s\nbenchRepoPath = %s" %
              (self.repoPath, self.srcRepoPath, self.benchRepoPath))


class ReportParameters:
    logFolder = r"Reports/"

    def __init__(self, cfgParser):
        self.logFolder = r"%s" % (cfgParser.get("ReportParameters", "logFolder"))

    def PrintParameters(self):
        print("Report parameters:\nlogFolder = %s" % self.logFolder)


class GeneralParameters:
    maxTimeForBenchmark = 3600  #one hour
    benchmarkCheckoutPath = r"../Benchmarks/"
    checkoutPath = r"../itlPlace/"
    buildLog = checkoutPath + r"obj/release/BuildLog.htm"
    slnPath = checkoutPath + r"make/itlPlace.sln"
    binDir = checkoutPath + r"bin/"

    def __init__(self, cfgParser):
        self.maxTimeForBenchmark = cfgParser.getint("GeneralParameters", "maxTimeForBenchmark")
        self.benchmarkCheckoutPath = os.path.join(CfgParserFactory.get_root_dir(), r"%s" % (cfgParser.get("GeneralParameters", "benchmarkCheckoutPath")))
        self.checkoutPath = os.path.join(CfgParserFactory.get_root_dir(), r"%s" % (cfgParser.get("GeneralParameters", "checkoutPath")))
        self.buildLog = os.path.join(CfgParserFactory.get_root_dir(), r"%s" % (cfgParser.get("GeneralParameters", "buildLog")))
        self.slnPath = os.path.join(CfgParserFactory.get_root_dir(), r"%s" % (cfgParser.get("GeneralParameters", "slnPath")))
        self.binDir = os.path.join(CfgParserFactory.get_root_dir(), r"%s" % (cfgParser.get("GeneralParameters", "binDir")))

    def PrintParameters(self):
        print("General parameters:\nmaxTimeForBenchmark = %d\nbenchmarkCheckoutPath = %s\ncheckoutPath = \
          %s\nbuildLog = %s\nslnPath = %s\nbinDir = %s" % (self.maxTimeForBenchmark,
                                                           self.benchmarkCheckoutPath, self.checkoutPath, self.buildLog,
                                                           self.slnPath, self.binDir))


class LogParserParameters:
    PFSTTableHeader = "Placement Flow Stages Table"
    PQATTableHeader = "Placement Quality Analysis Table"
    PFSTBorderPattern = "-----"

    def __init__(self, cfgParser):
        self.PFSTTableHeader = cfgParser.get("LogParserParameters", "PFSTTableHeader")
        self.PQATTableHeader = cfgParser.get("LogParserParameters", "PQATTableHeader")
        self.PFSTBorderPattern = cfgParser.get("LogParserParameters", "PFSTBorderPattern")

    def PrintParameters(self):
        print("LogParser parameters:\nPFSTTableHeader = %s\nPQATTableHeader = %s\nPFSTBorderPattern = %s" %
              (self.PFSTTableHeader, self.PQATTableHeader, self.PFSTBorderPattern))


class Tools:
    MSBuild = r"C:/WINDOWS/Microsoft.NET/Framework/v2.0.50727/MSBuild.exe"
    UnRar = r"7z.exe"

    def __init__(self, cfgParser):
        self.MSBuild = r"%s" % (cfgParser.get("Tools", "MSBuild"))
        self.UnRar = r"%s" % (cfgParser.get("Tools", "UnRar"))

    def PrintParameters(self):
        print("Tools:\nMSBuild = %s\nUnRar = %s" % (self.MSBuild, self.UnRar))


def test():
    configFile = "Parameters.conf"
    parentDir = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir))
    configFile = os.path.join(parentDir, configFile)
    cfgParser = ConfigParser()
    cfgParser.read(configFile)

    try:
        testRunnerParameters = TestRunnerParameters(cfgParser)
        testRunnerParameters.PrintParameters()

        emailerParameters = EmailerParameters(cfgParser)
        emailerParameters.PrintParameters()

        repoParameters = RepoParameters(cfgParser)
        repoParameters.PrintParameters()

        reportParameters = ReportParameters(cfgParser)
        reportParameters.PrintParameters()

        generalParameters = GeneralParameters(cfgParser)
        generalParameters.PrintParameters()

        logParserParameters = LogParserParameters(cfgParser)
        logParserParameters.PrintParameters()

        tools = Tools(cfgParser)
        tools.PrintParameters()

    except Exception:
        print("Error while parsing TestRunner parameters:")
        traceback.print_exc()


if __name__ == "__main__":
    test()
