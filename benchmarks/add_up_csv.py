import string
import csv
from argparse import ArgumentParser
import pandas as pd

parser = ArgumentParser()
parser.add_argument('-n', '--names-list', nargs='+', default=[])
files = parser.parse_args()

node1 = pd.read_csv(files.names_list[0], sep=",")
node2 = pd.read_csv(files.names_list[1], sep=",")
node3 = pd.read_csv(files.names_list[2], sep=",")

node1["reads"] = node1["reads"] + node2["reads"] + node3["reads"]
node1["appends"] = node1["appends"] + node2["appends"] + node3["appends"]
node1["ops"] = node1["ops"] + node2["ops"] + node3["ops"]

node1.to_csv("mergedCSV.csv")
