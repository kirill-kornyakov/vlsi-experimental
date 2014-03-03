import time
import os

from CoreFunctions import GetTimeStamp
from CfgParserFactory import CfgParserFactory


class LoggingLevel:
    DEBUG = 0
    RELEASE = 1

loggingLevel = LoggingLevel.RELEASE

class Logger:
    def __init__(self):
        cfgParser = CfgParserFactory.createCfgParser()
        selfLogFolder = CfgParserFactory.get_root_dir() + cfgParser.get("ReportParameters", "selfLogFolder")
        if not os.path.exists(selfLogFolder):  #TODO: use ReportCreator.CreateLogFolder
            os.mkdir(selfLogFolder)

        self.startTime = time.time()
        self.logFileName = selfLogFolder + r"TF_%s.log" % (GetTimeStamp())

    def LogWorkTime(self):
        runTime = time.time() - self.startTime
        self.Log("RUNTIME %2.2i:%2.2i" % (int(runTime / 60), int(runTime % 60)))

    def CoolLog(self, message):
        self.Log("\n")
        self.Log("######################################################")
        self.Log("####### %s" % message)
        self.Log("######################################################")
        self.LogWorkTime()
        self.Log("\n")

    def Log(self, message):
        message = ">> " + message

        print(message)

        log = open(self.logFileName, 'a')
        log.write("%s\n" % message)
        log.close()

    def LogD(self, message):
        message = ">> " + message

        if loggingLevel == LoggingLevel.DEBUG:
            print(message)

        log = open(self.logFileName, 'a')
        log.write("%s\n" % message)
        log.close()

    def LogStartMessage(self):
        self.CoolLog("Started on %s" % (GetTimeStamp()))


if __name__ == "__main__":
    pass
