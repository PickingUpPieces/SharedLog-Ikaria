import string
from argparse import ArgumentParser
import pandas as pd

parser = ArgumentParser()
parser.add_argument('-n', '--names-list', nargs='+', default=[])
files = parser.parse_args()

dfs = [pd.read_csv(csv) for csv in files]

node1 = pd.read_csv(files.names_list[0], sep=",")
node2 = pd.read_csv(files.names_list[1], sep=",")
node3 = pd.read_csv(files.names_list[2], sep=",")

for csv in dfs[1:]:
    dfs[0]["reads"] = dfs[0]["reads"] + csv["reads"] 
    dfs[0]["appends"] = dfs[0]["appends"] + csv["appends"] 
    dfs[0]["ops"] = dfs[0]["ops"] + csv["ops"] 

dfs[0].to_csv("merged-"+files.names_list[0], index=False)
