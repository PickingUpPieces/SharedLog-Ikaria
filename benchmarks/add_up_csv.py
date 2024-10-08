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
    dfs[0]["rops"] = round(dfs[0]["rops"] + csv["rops"], 0)
    dfs[0]["aops"] = round(dfs[0]["aops"] + csv["aops"], 0)
    dfs[0]["ops"] = round(dfs[0]["ops"] + csv["ops"], 0)
    dfs[0]["ops_node1"] = round(dfs[0]["ops_node1"] + csv["ops_node1"], 0)
    dfs[0]["ops_node2"] = round(dfs[0]["ops_node2"] + csv["ops_node2"], 0)
    dfs[0]["ops_node3"] = round(dfs[0]["ops_node3"] + csv["ops_node3"], 0)
    dfs[0]["ops_node4"] = round(dfs[0]["ops_node4"] + csv["ops_node4"], 0)
    dfs[0]["ops_node5"] = round(dfs[0]["ops_node5"] + csv["ops_node5"], 0)

dfs[0].to_csv("merged-"+files.names_list[0], index=False)
