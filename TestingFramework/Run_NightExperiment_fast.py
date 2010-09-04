import BaseChecker
from BaseChecker import *

lists = []
fast_lists = []

def NightExperiment():
    exp_HPWL_ISPD04 = BaseChecker('ISPD04 HPWL experiment', 'hpwl_ispd04.cfg', 'ISPD04_fast.list', "MasterLogs/HPWL/ISPD", ['HPWL'], ['DP'])
    exp_HPWL_IWLS05 = BaseChecker('IWLS05 HPWL experiment', 'hpwl_iwls05.cfg', 'IWLS05_fast.list', "MasterLogs/HPWL/IWLS", ['HPWL'], ['DP'])

    LRStages  = ['HPWL', 'TWL', 'TNS', 'WNS']
    LRMetrics = ['INIT', 'LEG']
    LRCmdArgs = '--LR.GlobalPlacement.LagrangianRelaxation.alphaTWL=1.0e-5'
    exp_LR_IWLS05GP = BaseChecker('IWLS05GP LR experiment', 'LR.cfg', 'IWLS_GP_r1511/IWLS_GP_fast.list', "MasterLogs/LR", LRStages, LRMetrics, LRCmdArgs)

    buffMetrics = ['HPWL', 'TWL', 'TNS', 'WNS']
    buffStages  = ['INIT', 'NBUF']
    exp_BUF = BaseChecker('IWLS05 new_buffering experiment (GBL)', 'New_buffering.cfg', 'IWLS05.list', "MasterLogs/New_Buffering/IWLS", buffMetrics, buffStages)

    hippMetrics = ['HPWL', 'TNS', 'WNS']
    hippStages  = ['LEG', 'DP']
    exp_HDP = BaseChecker('HippocrateDP (HDP) experiment', 'HippocrateDP.cfg', 'IWLS_GP_Hippocrate.list', "MasterLogs/HippocrateDP/Aleksandr", hippMetrics, hippStages)

    weightingMetrics = ['HPWL', 'TNS', 'WNS']
    weightingStages  = ['GP1', 'LEG1', 'GP9', 'LEG9']
    exp_WEIGHTING    = BaseChecker('Weighting (SGNW) experiment', 'Sensitivity_guided_weighting.cfg', 'IWLS05_fast.list', "MasterLogs/Weighting/SensitivityGuided", weightingMetrics, weightingStages)

    nightTestParams = TestRunnerParameters()
    nightTestParams.doCheckout = False
    nightTestParams.doBuild    = False
    nightTestParams.doSendMail = False
    testRunner = TestRunner(nightTestParams)
    #testRunner.parameters.experiments.append(exp_HPWL_IWLS05)
    #testRunner.parameters.experiments.append(exp_HPWL_ISPD04)
    testRunner.parameters.experiments.append(exp_LR_IWLS05GP)
    #testRunner.parameters.experiments.append(exp_BUF)
    #testRunner.parameters.experiments.append(exp_HDP)
    #testRunner.parameters.experiments.append(exp_WEIGHTING)
    testRunner.Run()

def run():
    NightExperiment() #<-lists


def fast_run():
    NightExperiment() #<-fast_lists

run()
#fast_run()
