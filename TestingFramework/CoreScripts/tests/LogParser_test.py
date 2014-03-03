import os
from CoreScripts.LogParser import *

# Placement Flow Stages Table
# StageName    Tag  Time(sec) Cells HPWL(nm)  TWL(nm) TNS(ns) WNS(ns)
# -------------------------------------------------------------------
# InitialState INIT     3,373   141  1132158  1132158  8,3682  0,6336
# Buffering    BUF      3,721   154  1132158  1132158  8,4757  0,6269
# Legalization LEGB     4,043   154 11815340 12011690 11,9258  0,9495

def test_GetFromTable():
    log_name = os.path.dirname(__file__) + '/sample.log'
    parser = LogParser(log_name)

    assert parser.GetFromTable('INIT', 'HPWL') == "1132158"
    assert parser.GetFromTable('INIT', 'TWL') == "1132158"
    assert parser.GetFromTable('INIT', 'TNS') == "8,3682"
    assert parser.GetFromTable('INIT', 'WNS') == "0,6336"
