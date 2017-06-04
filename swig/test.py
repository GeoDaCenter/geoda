from geoda import VecDouble, VecInt, VecVecDouble, VecFloat, VecString
import geoda

import pysal
dbf = pysal.open('../SampleData/nat.dbf','r')
x = dbf.by_col('HR60')
x1 = dbf.by_col('HR70')
n = 3085

print "====================="
print "PCA"
xx = []
x_names = ['HR60', 'HR70']
for i in range(n):
    xx.append(x[i])
    xx.append(x1[i])
var_1 = VecFloat(xx)
rtn = geoda.PCA(var_1, VecString(x_names), n, 2, 0, 1, 1);

print rtn

print "====================="
print "hinge15"
var_1 = VecDouble(x)

breaks = VecDouble([0 for i in range(6)])
geoda.Hinge15( n, var_1, 6, False, breaks)
for i in range(6):
    print breaks[i]

breaks = VecDouble([0 for i in range(6)])
geoda.Hinge30( n, var_1, 6, False, breaks)

for i in range(6):
    print breaks[i]

print "====================="
print "local geary"
localMoran = VecDouble([0 for i in range(n)])
sigLocalMoran = VecDouble([0 for i in range(n)])
sigFlag = VecInt([0 for i in range(n)])
clusterFlag = VecInt([0 for i in range(n)])
numPermutation = 599

var = VecVecDouble([x,x1])
geoda.LocalGeary("nat.gal", var, localMoran, sigLocalMoran, sigFlag, clusterFlag, numPermutation)

for i in range(10):
    print localMoran[i], clusterFlag[i], sigLocalMoran[i], sigFlag[i]

print "====================="
print "lisa"
var_1 = VecDouble(x)
#var_1 = VecDouble([i for i in x])
var_2 = VecDouble([0 for i in range(n)])
localMoran = VecDouble([0 for i in range(n)])
sigLocalMoran = VecDouble([0 for i in range(n)])
sigFlag = VecInt([0 for i in range(n)])
clusterFlag = VecInt([0 for i in range(n)])
lisa_type = 0
numPermutation = 599
geoda.LISA("nat.gal", var_1, var_2, localMoran, sigLocalMoran, sigFlag, clusterFlag, lisa_type, numPermutation)

for i in range(10):
    print localMoran[i], clusterFlag[i], sigLocalMoran[i], sigFlag[i]

print "====================="
geoda.CreateQueenWeights("nat.shp","nat.gal",1,False)

geoda.CreateRookWeights("nat.shp","nat_rook.gal",1,False)

geoda.CreateKNNWeights("nat.shp","nat_knn.gal", 4)

geoda.CreateKNNWeights("nat.shp","nat_knn.gal", 4, True, False)

geoda.CreateDistanceWeights("nat.shp","nat_dist.gal", 1.465776)

