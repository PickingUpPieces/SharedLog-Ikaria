import string
import csv
from argparse import ArgumentParser

parser = ArgumentParser()
parser.add_argument('-n', '--names-list', nargs='+', default=[])
files = parser.parse_args()

merged = open('merged.csv', 'w')
writer = csv.writer(merged)

for f in files.names_list[1:]: 
    with open('minitest.csv', 'rb') as f:
    reader = csv.reader(f)
    data = [next(reader)]  # title row
    for row in reader:
        data.append(row + [float(row[2]) * float(row[5])])
            for line1, line2 in zip(file1, file2):
		        line1Columns = line1.split(",") 
		        line2Columns = line2.split(",") 
                writer.writerow(value)