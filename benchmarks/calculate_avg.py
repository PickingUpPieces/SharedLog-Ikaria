import string
from argparse import ArgumentParser
import pandas as pd

parser = ArgumentParser()
parser.add_argument('-n', '--names-list', nargs='+', default=[])
files = parser.parse_args()

dfs = [pd.read_csv(csv) for csv in files.names_list]

for csv in dfs[1:]:
    dfs[0]["reads"] = dfs[0]["reads"] + csv["reads"] 
    dfs[0]["appends"] = dfs[0]["appends"] + csv["appends"] 
    dfs[0]["ops"] = dfs[0]["ops"] + csv["ops"] 
    dfs[0]["rops"] = dfs[0]["rops"] + csv["rops"] 
    dfs[0]["aops"] = dfs[0]["aops"] + csv["aops"] 
    dfs[0]["alat_50"] = dfs[0]["alat_50"] + csv["alat_50"] 
    dfs[0]["alat_95"] = dfs[0]["alat_99"] + csv["alat_99"] 
    dfs[0]["alat_99"] = dfs[0]["alat_99"] + csv["alat_99"] 
    dfs[0]["rlat_50"] = dfs[0]["rlat_50"] + csv["rlat_50"] 
    dfs[0]["rlat_95"] = dfs[0]["rlat_95"] + csv["rlat_95"] 
    dfs[0]["rlat_99"] = dfs[0]["rlat_99"] + csv["rlat_99"] 

dfs[0]["reads"] = round(dfs[0]["reads"] / len(dfs), 0)
dfs[0]["appends"] = round(dfs[0]["appends"] / len(dfs), 0)
dfs[0]["ops"] = round(dfs[0]["ops"] / len(dfs), 0)
dfs[0]["aops"] = round(dfs[0]["aops"] / len(dfs), 0)
dfs[0]["rops"] = round(dfs[0]["rops"] / len(dfs), 0)
dfs[0]["alat_50"] = round(dfs[0]["alat_50"] / len(dfs), 0)
dfs[0]["alat_95"] = round(dfs[0]["alat_95"] / len(dfs), 0)
dfs[0]["alat_99"] = round(dfs[0]["alat_99"] / len(dfs), 0)
dfs[0]["rlat_50"] = round(dfs[0]["rlat_50"] / len(dfs), 0)
dfs[0]["rlat_95"] = round(dfs[0]["rlat_95"] / len(dfs), 0)
dfs[0]["rlat_99"] = round(dfs[0]["rlat_99"] / len(dfs), 0)

dfs[0].to_csv("average-"+files.names_list[0], index=False)
