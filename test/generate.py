import pandas as pd

def create_ascii_map(numbers, x, y):
    ascii_map = []
    num_index = 0
    for i in range(y):
        row = []
        for j in range(x):
            if num_index >= len(numbers):
                num_index = 0
            row.append(numbers[num_index])
            num_index += 1
        ascii_map.append(row)
    return ascii_map

def print_ascii_map(ascii_map):
    for row in ascii_map:
        print(' '.join(str(cell) for cell in row))

def create_asc_file(ascii_map, filename):
    with open(filename, 'w') as f:
        for row in ascii_map:
            f.write(' '.join(str(cell) for cell in row) + '\n')

fbp = pd.read_csv("FBP_Instance/fbp_lookup_table.csv")
fbp_fuels = fbp.iloc[:, 0].tolist()

kitral = pd.read_csv("K_Instance/kitral_lookup_table.csv")
kitral_fuels = kitral.iloc[:, 0].tolist()

sb = pd.read_csv("SB_Instance/spain_lookup_table.csv")
sb_fuels = sb.iloc[:, 0].tolist()

print("cantidad de fuels FBP:",len(fbp_fuels))
print("cantidad de fuels KITRAL:",len(kitral_fuels))
print("cantidad de fuels SB:",len(sb_fuels))


# Example usage
sb_map = create_ascii_map(sb_fuels, 7, 7)
filename = 'sb_fuels.asc'
create_asc_file(sb_map, filename)

fbp_map = create_ascii_map(fbp_fuels, 12, 12)
filename = 'fbp_fuels.asc'
create_asc_file(fbp_map, filename)

k_map = create_ascii_map(kitral_fuels, 7, 7)
filename = 'k_fuels.asc'
create_asc_file(k_map, filename)