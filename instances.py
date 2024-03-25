import pandas as pd

df = pd.read_csv("data/CanadianFBP/400cellsC1/fbp_lookup_table.csv", header=1)
lista = list(df.columns.values)
print(df)