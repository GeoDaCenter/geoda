from geoda import VecDouble, VecInt
import geoda

n = 3085
var_1 = VecDouble([i for i in range(n)])
var_2 = VecDouble([0 for i in range(n)])
localMoran = VecDouble([0 for i in range(n)])
sigLocalMoran = VecDouble([0 for i in range(n)])
sigFlag = VecInt([0 for i in range(n)])
clusterFlag = VecInt([0 for i in range(n)])
lisa_type = 0
numPermutation = 599

geoda.LISA("/home/xun/downloads/geoda/swig/nat.gal", var_1, var_2, localMoran, sigLocalMoran, sigFlag, clusterFlag, lisa_type, numPermutation)

geoda.CreateQueenWeights("nat.shp","nat.gal",1,False)

geoda.CreateRookWeights("nat.shp","nat_rook.gal",1,False)

geoda.CreateKNNWeights("nat.shp","nat_knn.gal", 4)

geoda.CreateKNNWeights("nat.shp","nat_knn.gal", 4, True, False)

geoda.CreateDistanceWeights("nat.shp","nat_dist.gal", 1.465776)

