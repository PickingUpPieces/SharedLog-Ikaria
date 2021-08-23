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

dfs[0].to_csv("merged-"+files.names_list[0], index=False)