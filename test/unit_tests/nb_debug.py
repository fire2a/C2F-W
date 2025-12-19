#!python3
import numpy as np
from matplotlib.pyplot import plot, show

# testing head_cell functions


@np.vectorize
def func1(x):
    return (int(180 - x) + 360) % 360


@np.vectorize
def func2(x):
    a = x - 90
    if a > 0:
        a += 360
    return a


circle = np.linspace(0, 360, num=3600, endpoint=False)

# for c in circle:
#     print(f"func({c}) = {func(c)}")

plot(circle, func1(circle))
plot(circle, func2(circle))
show()

# testing nb to angle


def nb_angle1(base, neighbor):
    """original code"""
    a, b = base - neighbor
    if a == 0:
        if b >= 0:
            return 270
        else:
            return 90
    elif b == 0:
        if a >= 0:
            return 180
        else:
            return 0
    # print("1")
    return nb_angle2(base, neighbor)


def nb_angle2(base, neighbor):
    """only the second part of original code"""
    a, b = base - neighbor
    tmp = np.atan(b / a) * 180 / np.pi
    # print(a,b,tmp)
    if a > 0:
        # print("a>0 +180")
        # print("2")
        tmp += 180
    if a < 0 and b > 0:
        # print("a<0 b>0 +360")
        # print("3")
        tmp += 360
    return tmp


def nb_angle3(base, neighbor):
    """suggested fix"""
    a, b = base - neighbor
    return (np.arctan2(-b, -a) * 180 / np.pi + 360) % 360


base = np.array([2, 2])
neighbors = np.meshgrid(np.arange(5), np.arange(5))

print("base, neighbor, angle1, angle2, angle3".upper())
for nb in zip(neighbors[0].flatten(), neighbors[1].flatten()):
    angle1 = nb_angle1(base, np.array(nb))
    angle2 = nb_angle2(base, np.array(nb))
    angle3 = nb_angle3(base, np.array(nb))
    print(f"base: {base} neighbor: {list(map(int,nb))} angle: {angle1:6.2f}, {angle2:6.2f}, {angle3:6.2f}")
