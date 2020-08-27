#include <algorithm>
#include "azp.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ZoneControl
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
ZoneControl<T>::ZoneControl(const std::vector<T>& in_data)
: data(in_data)
{
}

template <typename T>
ZoneControl<T>::~ZoneControl()
{
}

template <typename T>
void ZoneControl<T>::AddControl(Operation op, Comparator cmp, const T& val)
{
    operations.push_back(op);
    comparators.push_back(cmp);
    comp_values.push_back(val);
}

template <typename T>
bool ZoneControl<T>::CheckZone(const std::vector<int>& candidates)
{
    bool is_valid = true; // default true since no check will yield good cands

    for (size_t i=0;  i< comparators.size(); ++i) {
        // get zone value for comparison
        double zone_val = 0;
        if (operations[i] == Operation::SUM) {
            double sum = 0;
            for (int j=0; j<candidates.size(); ++j) {
                sum += data[ candidates[j] ];
            }
            zone_val = sum;
        } else if (operations[i] == Operation::MEAN) {
            double sum = 0;
            for (int j=0; j<candidates.size(); ++j) {
                sum += data[candidates[j]];
            }
            double mean = sum / (double) candidates.size();
            zone_val = mean;
        } else if (operations[i] == Operation::MAX) {
            double max = data[candidates[0]];
            for (int j=1; j<candidates.size(); ++j) {
                if (max < data[candidates[j]]) {
                    max = data[candidates[j]];
                }
            }
            zone_val = max;
        } else if (operations[i] == Operation::MIN) {
            double min = data[candidates[0]];
            for (int j=1; j<candidates.size(); ++j) {
                if (min > data[candidates[j]]) {
                    min = data[candidates[j]];
                }
            }
            zone_val = min;
        }

        // compare zone value
        if (comparators[i] == Comparator::LESS_THAN) {
            if (zone_val > comp_values[i]) {
                return false;
            }
        } else if (comparators[i] == Comparator::MORE_THAN) {
            if (zone_val < comp_values[i]) {
                return false;
            }
        }
    }
    return is_valid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AreaManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
AreaManager<T>::AreaManager(int in_n, GalElement* const in_w, DistMatrix* const _dist_matrix)
: n(in_n), w(in_w), dist_matrix(_dist_matrix)
{
}

template <typename T>
double AreaManager<T>::returnDistance2Area(int i, int j)
{
    return dist_matrix->getDistance(i, j);
}

template <typename T>
std::vector<double> AreaManager<T>::getDataAverage(const std::vector<int>& areaList)
{
    return std::vector<double>();
}

template <typename T>
double AreaManager<T>::getDistance2Region(int area, const std::set<int>& areaList)
{
    // get centroid from a areaList (region)
    std::vector<double> centroidRegion(m, 0);
    std::set<int>::iterator it;
    for (it = areaList.begin(); it != areaList.end(); ++it) {
        int area_id = *it;
        for (int j=0; j<m; ++j) {
            centroidRegion[j] += data[area_id];
        }
    }
    for (int j=0; j<m; ++j) {
        centroidRegion[j] /= (double)areaList.size();
    }
    // get distance between area and region
    double dist = DataUtils::EuclideanDistance(data[area], centroidRegion);

    return dist;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// RegionMaker
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
RegionMaker::RegionMaker()
{
    std::vector<int>  seeds = this->kmeansInit();
    this->setSeeds(seeds);
    int c = 0;
    while (unassignedAreas.size() != 0) {
        constructRegions();
        c += 1;
    }
}

void RegionMaker::AssignAreasNoNeighs()
{
    // w should not be NULL
    for (int i=0; i<n; ++i) {
        if (w[i].Size() == 0) {
            areaNoNeighbor[i] = true;
            assignedAreas.push_back(i);
        } else {
            areaNoNeighbor[i] = false;
            unassignedAreas.push_back(i);
        }
    }
}

std::vector<int> RegionMaker::kmeansInit()
{
    //Initial p starting observations using K-Means
    std::vector<double> probabilities(n, p);
    std::vector<double> distances(n, 1);
    double total = n;
    for (int j=0; j<n; ++j) {
        probabilities[j] = distances[j] / total;
    }

    std::vector<int> seeds;
    for (int i=0; i < p; ++i) {
        double random = rng.nextDouble();
        bool find = false;
        int acum = 0, cont = 0;
        while (find == false) {
            double inf = acum;
            double sup = acum + probabilities[cont];
            if (inf <= random && random <= sup) {
                find = true;
                seeds.push_back(cont);
                // for each observation/area, find which seed is the closest
                total = 0;
                for (int j=0; j<n; ++j) {
                    double distancei = 0;
                    for (int k=0; k<seeds.size(); ++k) {
                        double d = dist_matrix->getDistance(j, seeds[k]);
                        if (k==0) distancei = d;
                        else if (distancei < d) {
                            distancei = d;
                        }
                    }
                    total += distancei; // get a sum of new distances
                    distances[j] = distancei;
                }
                for (int j=0; j<n; ++j) {
                    probabilities[j] = distances[j] / total;
                }
            } else {
                cont += 1;
                acum = sup;
            }
        }
    }
    return seeds;
}

void RegionMaker::setSeeds(const std::vector<int>& seeds)
{
    this->seeds.clear();

    int ns = seeds.size() < p ? seeds.size() : p;

    for (int i=0; i<ns; ++i) {
        this->seeds.push_back(seeds[i]);
    }

    if (this->seeds.size() < p) {
        // pick more to fill up seeds
        std::vector<int> didx(n, true);
        // remove seeds, and neighborless observations
        for (int i=0; i<seeds.size(); ++i) {
            didx[ seeds[i] ] = false;
        }
        for (int i=0; i<n; ++i) {
            if (w[i].Size() == 0) {
                didx[i] = false;
            }
        }
        std::vector<int> cands;
        for (int i=0; i<didx.size(); ++i) {
            if (didx[i] == true) {
                cands.push_back(i);
            }
        }
        DataUtils::Shuffle(cands, rng);

        for (int i=0; i<p - this->seeds.size(); ++i) {
            this->seeds.push_back(cands[i]);
        }
    }

    int c = 0;
    for (int i=0; i<this->seeds.size(); ++i) {
        // self.NRegion += [0]
        assignSeeds(this->seeds[i], c);
        c += 1;
    }
}

void RegionMaker::assignSeeds(int areaID, int regionID)
{
    assignAreaStep1(areaID, regionID);

    // check neighbors of areaID
    const std::vector<long>& nbrs = w[areaID].GetNbrs();
    for (int i=0; i<nbrs.size(); ++i) {
        int neigh = nbrs[i];
        potentialRegions4Area[neigh].insert(regionID);
    }

    // remove self from potentialRegions4Area
    potentialRegions4Area.erase(areaID);

    //self.changedRegion = 'null'
    //self.newExternal = self.potentialRegions4Area.keys()
}

void RegionMaker::assignAreaStep1(int areaID, int regionID)
{
    region2Area[regionID].insert(areaID);
    area2Region[areaID] = regionID;

    unassignedAreas.erase(std::find(unassignedAreas.begin(), unassignedAreas.end(), areaID));
    assignedAreas.push_back(areaID);
}

void RegionMaker::constructRegions()
{
    int lastRegion = 0;
    std::map<int, std::set<int> >::iterator it;
    std::set<int>::iterator rit, ait;

    for (it=potentialRegions4Area.begin(); it != potentialRegions4Area.end(); ++it) {
        int areaID = it->first;
        // compute distance from areaID to different regions
        std::set<int> regionIDs = it->second;
        for (rit = regionIDs.begin(); rit != regionIDs.end(); ++rit) {
            int region = *rit;
            std::set<int> areasIdsIn = region2Area[region];
            double regionDistance = am.getDistance2Region(areaID, areasIdsIn);
            this->candidateInfo[std::make_pair(areaID, region)] = regionDistance;
        }
    }

    //Select and assign the nearest area to a region
    
}
