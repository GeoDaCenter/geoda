#include <algorithm>
#include <queue>
#include <iterator>
#include <stack>

#include "DataUtils.h"
#include "azp.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ZoneControl
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ZoneControl::ZoneControl(const std::vector<double>& in_data)
: data(in_data)
{
}

ZoneControl::~ZoneControl()
{
}

void ZoneControl::AddControl(Operation op, Comparator cmp, const double& val)
{
    operations.push_back(op);
    comparators.push_back(cmp);
    comp_values.push_back(val);
}

bool ZoneControl::CheckRemove(int area, boost::unordered_map<int, bool>& candidates)
{
    bool is_valid = true; // default true since no check will yield good cands
    boost::unordered_map<int, bool>::iterator it;
    for (size_t i=0;  i< comparators.size(); ++i) {
        // get zone value for comparison
        double zone_val = 0;
        if (operations[i] == Operation::SUM) {
            double sum = 0;
            for (it=candidates.begin(); it!=candidates.end(); ++it) {
                sum += data[ it->first ];
            }
            sum -= data[area];
            zone_val = sum;
        } else if (operations[i] == Operation::MEAN) {
            double sum = 0;
            for (it=candidates.begin(); it!=candidates.end(); ++it) {
                sum += data[it->first];
            }
            sum -= data[area];
            double mean = sum / (double) (candidates.size() - 1);
            zone_val = mean;
        } else if (operations[i] == Operation::MAX) {
            double max = data[candidates[0]];
            for (it=candidates.begin(); it!=candidates.end(); ++it) {
                if (max < data[it->first] && it->first != area) {
                    max = data[it->first];
                }
            }
            zone_val = max;
        } else if (operations[i] == Operation::MIN) {
            double min = data[candidates[0]];
            for (it=candidates.begin(); it!=candidates.end(); ++it) {
                if (min > data[it->first] && it->first != area) {
                    min = data[it->first];
                }
            }
            zone_val = min;
        }

        // compare zone value
        if (comparators[i] == Comparator::MORE_THAN) {
            if (zone_val <= comp_values[i]) {
                return false;
            }
        }
    }
    return is_valid;
}

bool ZoneControl::CheckAdd(int area, boost::unordered_map<int, bool>& candidates)
{
    bool is_valid = true; // default true since no check will yield good cands
    boost::unordered_map<int, bool>::iterator it;
    for (size_t i=0;  i< comparators.size(); ++i) {
        // get zone value for comparison
        double zone_val = 0;
        if (operations[i] == Operation::SUM) {
            double sum = 0;
            for (it=candidates.begin(); it!=candidates.end(); ++it) {
                sum += data[ it->first ];
            }
            sum += data[area];
            zone_val = sum;
        } else if (operations[i] == Operation::MEAN) {
            double sum = 0;
            for (it=candidates.begin(); it!=candidates.end(); ++it) {
                sum += data[it->first];
            }
            sum += data[area];
            double mean = sum / (double) (candidates.size() + 1);
            zone_val = mean;
        } else if (operations[i] == Operation::MAX) {
            double max = data[candidates[0]];
            for (it=candidates.begin(); it!=candidates.end(); ++it) {
                if (max < data[it->first]) {
                    max = data[it->first];
                }
            }
            if (max < data[area]) {
                max = data[area];
            }
            zone_val = max;
        } else if (operations[i] == Operation::MIN) {
            double min = data[candidates[0]];
            for (it=candidates.begin(); it!=candidates.end(); ++it) {
                if (min > data[it->first]) {
                    min = data[it->first];
                }
            }
            if (min > data[area]) {
                min = data[area];
            }
            zone_val = min;
        }

        // compare zone value
        if (comparators[i] == Comparator::LESS_THAN) {
            if (zone_val >= comp_values[i]) {
                return false;
            }
        }
    }
    return is_valid;
}

bool ZoneControl::CheckCandidate(int area, boost::unordered_map<int, bool>& candidates, bool upperbound_only)
{
    bool is_valid = true; // default true since no check will yield good cands
    boost::unordered_map<int, bool>::iterator it;

    for (size_t i=0;  i< comparators.size(); ++i) {
        if (upperbound_only && comparators[i] != Comparator::LESS_THAN) {
            continue;
        }

        // get zone value for comparison
        double zone_val = 0;
        if (operations[i] == Operation::SUM) {
            double sum = 0;
            for (it=candidates.begin(); it!=candidates.end(); ++it) {
                sum += data[ it->first ];
            }
            sum += data[area];
            zone_val = sum;
        } else if (operations[i] == Operation::MEAN) {
            double sum = 0;
            for (it=candidates.begin(); it!=candidates.end(); ++it) {
                sum += data[it->first];
            }
            sum += data[area];
            double mean = sum / (double) (candidates.size() + 1);
            zone_val = mean;
        } else if (operations[i] == Operation::MAX) {
            double max = data[candidates[0]];
            for (it=candidates.begin(); it!=candidates.end(); ++it) {
                if (max < data[it->first]) {
                    max = data[it->first];
                }
            }
            if (max < data[area]) {
                max = data[area];
            }
            zone_val = max;
        } else if (operations[i] == Operation::MIN) {
            double min = data[candidates[0]];
            for (it=candidates.begin(); it!=candidates.end(); ++it) {
                if (min > data[it->first]) {
                    min = data[it->first];
                }
            }
            if (min > data[area]) {
                min = data[area];
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

AreaManager::AreaManager(int _n, int _m, GalElement* const _w, double** _data, DistMatrix* const _dist_matrix)
: n(_n), m(_m), w(_w), data(_data), dist_matrix(_dist_matrix)
{
}

double AreaManager::returnDistance2Area(int i, int j)
{
    return dist_matrix->getDistance(i, j);
}

std::vector<double> AreaManager::getDataAverage(const std::set<int>& areaList)
{
    return std::vector<double>();
}

double AreaManager::getDistance2Region(int area, int region, REGION_AREAS& regions)
{
    std::vector<double> d(m,0);
    for (int i=0; i<m; ++i) d[i] = data[area][i];

    // get centroid of region
    if (region_centroids.find(region) == region_centroids.end()) {
        updateRegionCentroids(region, regions);
    }
    std::vector<double>& centroidRegion = region_centroids[region];

    // get distance between area and region
    double dist = DataUtils::EuclideanDistance(d, centroidRegion);

    return dist;
}

void AreaManager::updateRegionCentroids(int region, REGION_AREAS& regions)
{
    boost::unordered_map<int, bool>& areaList = regions[region];
    std::vector<double> centroid(m, 0);
    boost::unordered_map<int, bool>::iterator it;
    for (it = areaList.begin(); it != areaList.end(); ++it) {
        int area_id = it->first;
        for (int j=0; j<m; ++j) {
            centroid[j] += data[area_id][j];
        }
    }
    for (int j=0; j<m; ++j) {
        centroid[j] /= (double)areaList.size();
    }
    region_centroids[region] = centroid;
}

////////////////////////////////////////////////////////////////////////////////
////// RegionMaker
////////////////////////////////////////////////////////////////////////////////
RegionMaker::RegionMaker(int _p, GalElement* const _w,
                         double** _data, // row-wise
                         RawDistMatrix* _dist_matrix,
                         int _n, int _m, const std::vector<ZoneControl>& c)
: p(_p), w(_w), data(_data), dist_matrix(_dist_matrix), n(_n), m(_m), controls(c),
am(_n, _m, _w, _data, _dist_matrix), objInfo(-1)
{
    // init unassigned areas
    for (int i=0; i<n; ++i) {
        unassignedAreas[i] = true;
    }
    // mark neighborless as
    
    // get p start areas using k-means
    std::vector<int> seeds = this->kmeansInit();
    // for replicate clusterpy: CA_polygons
    // calif.cluster('azp', ['PCR2002'], 9, wType='rook', dissolve=1)
    // [31, 38, 37, 28, 20, 29, 17, 50, 55]
    //seeds[0] = 31; seeds[1] = 38; seeds[2] = 37; seeds[3]=28; seeds[4]=20;
    //seeds[5] = 29; seeds[6] = 17; seeds[7] = 50; seeds[8] = 55;

    // process and assign the neighbors of p starting areas
    this->setSeeds(seeds);

    // for any other unassigned areas, assign region? parallel?
    while (unassignedAreas.size() != 0) {
        this->constructRegions();
    }

    //  create objectiveFunction object for local improvement
    objective_function = new ObjectiveFunction(n, m, data, w, region2Area);

    // get objective function value
    this->objInfo = objective_function->GetValue();
}

RegionMaker::~RegionMaker()
{
    delete objective_function;
}

void RegionMaker::AssignAreasNoNeighs()
{
    // w should not be NULL
    for (int i=0; i<n; ++i) {
        if (w[i].Size() == 0) {
            areaNoNeighbor[i] = true;
            assignedAreas[i] = true;
        } else {
            areaNoNeighbor[i] = false;
            unassignedAreas[i] = true;
        }
    }
}

std::vector<int> RegionMaker::kmeansInit()
{
    //Initial p starting observations using K-Means
    std::vector<double> probabilities(n, 0);
    std::vector<double> distances(n, 1);
    double total = n;
    for (int j=0; j<n; ++j) {
        probabilities[j] = distances[j] / total;
    }

    std::vector<int> seeds;
    for (int i=0; i < p; ++i) {
        double random = rng.nextDouble();
        //std::cout << random << std::endl;
        bool find = false;
        double acum = 0;
        int cont = 0;
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
                        else if (d <  distancei) {
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
    // Sets the initial seeds for clustering
    this->seeds.clear();

    int ns = seeds.size() < p ? (int)seeds.size() : p;

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
    // Assign an area to a region and updates potential regions for the neighs
    // parameters
    assignAreaStep1(areaID, regionID);

    // Grow from the seed until satisfy the controls lower bounds
    std::stack<int> grow;
    grow.push(areaID);
    while (grow.empty() == false) {
        int a = grow.top();
        grow.pop();

        assignAreaStep1(a, regionID);

        bool stop_grow = false;
        for (int i=0; i<controls.size(); ++i) {
            if (controls[i].SatisfyLowerBound(region2Area[regionID])) {
                stop_grow = true;
            }
        }

        if (stop_grow) {
            break;
        }

        const std::vector<long>& nbrs = w[a].GetNbrs();
        for (int i=0; i<nbrs.size(); ++i) {
            int neigh = (int)nbrs[i];
            if (assignedAreas.find(neigh) == assignedAreas.end()) {
                grow.push(neigh);
            }
        }
    }

    //potentialRegions4Area = getBufferingAreas(regionID);

    // check neighbors of areaID that are not been assigned yet
    // and assign neighbor to potential regions
    const std::vector<long>& nbrs = w[areaID].GetNbrs();
    for (int i=0; i<nbrs.size(); ++i) {
        int neigh = (int)nbrs[i];
        if (assignedAreas.find(neigh) == assignedAreas.end()) {
            potentialRegions4Area[neigh].insert(regionID);
        }
    }

    // remove areaID from potentialRegions4Area, since it's processed
    potentialRegions4Area.erase(areaID);
}

void RegionMaker::assignAreaStep1(int areaID, int regionID)
{
    //  Assgin an area to a region
    region2Area[regionID][areaID] = false;

    // attach region with area
    area2Region[areaID] = regionID;

    // remove areaID from unassignedAreas
    unassignedAreas.erase(areaID);

    // add areaID to assignedAreas
    assignedAreas[areaID] = true;
}

void RegionMaker::constructRegions()
{
    // Construct potential regions per area (not assigned)
    int lastRegion = 0;
    boost::unordered_map<int, std::set<int> >::iterator it;
    std::set<int>::iterator rit, ait;

    // Process the most feasible area that has shortest distance to a region

    for (it=potentialRegions4Area.begin(); it != potentialRegions4Area.end(); ++it) {
        int areaID = it->first;
        const std::set<int>& regionIDs = it->second;

        for (rit = regionIDs.begin(); rit != regionIDs.end(); ++rit) {
            int region = *rit;
            std::pair<int, int> a_r = std::make_pair(areaID, region);
            double regionDistance = am.getDistance2Region(areaID, region, region2Area);
            // (areaID, region): distance
            candidateInfo[a_r] = regionDistance;
        }
    }

    if (candidateInfo.empty() == false)  {

        // Select and assign the nearest area to a region
        // minimumSelection(RegionMaker)
        // if there are more than one (area, region) pair
        // random select from cands

        std::vector<std::pair<int, int> > cands;
        boost::unordered_map<std::pair<int, int>, double>::iterator cit;
        
        // get min dist
        double min_region_distance = std::numeric_limits<double>::max();
        for (cit = candidateInfo.begin(); cit != candidateInfo.end(); ++cit) {
            if (cit->second < min_region_distance)  {
                min_region_distance = cit->second;
            }
        }
        // get all pairs with min dist
        for (cit = candidateInfo.begin(); cit != candidateInfo.end(); ++cit) {
            if (cit->second == min_region_distance) {
                cands.push_back(cit->first);
            }
        }

        // if random select candidate from pairs is needed
        int rnd_sel = cands.size() == 1? 0 : rng.nextInt((int)cands.size());
        std::pair<int, int>& sel_area_region = cands[rnd_sel];
        int aid = sel_area_region.first;
        int rid = sel_area_region.second;

        // assign select areaID to regionID, and process the neighbors of areaID
        if (this->assignArea(aid, rid)) {

            // remove from candidateInfo with areaID=aid
            std::vector<std::pair<int, int> > removed_cands;
            for (cit = candidateInfo.begin(); cit != candidateInfo.end(); ++cit) {
                if (cit->first.first == aid) {
                    removed_cands.push_back(cit->first);
                }
            }
            for (int i=0; i<removed_cands.size(); ++i) {
                candidateInfo.erase(removed_cands[i]);
            }
        } else {
            // can not assign, violate controls
            // just remove current pair
            candidateInfo.erase(sel_area_region);
        }
    }
}



bool RegionMaker::assignArea(int areaID, int regionID)
{

    //Assign an area to a region and updates potential regions for its neighs

    this->assignAreaStep1(areaID, regionID);

    //for neigh in self.neighsMinusAssigned:
    // assign regionID as a potential region for neigh
    const std::vector<long>& neighs = this->w[areaID].GetNbrs();
    for (int i=0; i< neighs.size(); ++i) {
        int nn = (int)neighs[i];
        if (assignedAreas.find(nn) == assignedAreas.end()) {
            // for not yet assigned neighbor
            //self.potentialRegions4Area[neigh] = self.potentialRegions4Area[neigh]|set([regionID])
            potentialRegions4Area[nn].insert(regionID);
        }
    }
    // remove areaID since its processed
    potentialRegions4Area.erase(areaID);

    // update centroid of the region
    am.updateRegionCentroids(regionID, region2Area);

    return true;
}

std::vector<int> RegionMaker::returnRegions()
{
    std::vector<int> areasId, results;
    boost::unordered_map<int, int>::iterator it;
    // area2Region is already sorted by key
    for (it = area2Region.begin(); it != area2Region.end(); ++it) {
        areasId.push_back( it->first );
    }
    std::sort(areasId.begin(), areasId.end());
    for (int i=0; i<areasId.size(); ++i) {
        results.push_back( area2Region[areasId[i] ]);
    }
    return results;
}

std::set<int> RegionMaker::getBufferingAreas(int regionID)
{
    std::set<int> buffering_areas;

    boost::unordered_map<int, bool>& areas  = region2Area[regionID];
    boost::unordered_map<int, bool>::iterator it;
    for (it = areas.begin(); it != areas.end(); ++it) {
        int area = it->first;
        const std::vector<long>& nn  = w[area].GetNbrs();
        for (int i=0; i<nn.size(); ++i) {
            if (areas.find((int)nn[i])  == areas.end()) {
                // neighbor not in this region, the the area is at border
                buffering_areas.insert((int)nn[i]);
            }
        }
    }

    return buffering_areas;
}

void RegionMaker::getBorderingAreas(int regionID)
{
    // check every area, and return who has neighbors out of the region
    boost::unordered_map<int, bool>& areas  = region2Area[regionID];
    boost::unordered_map<int, bool>::iterator it;
    for (it = areas.begin(); it != areas.end(); ++it) {
        int area = it->first;
        const std::vector<long>& nn  = w[area].GetNbrs();
        areas[area] = false;  // not a border area by default
        for (int i=0; i<nn.size(); ++i) {
            if (areas.find((int)nn[i])  == areas.end()) {
                // neighbor not in this region, the the area is at border
                areas[area] = true;
                break;
            }
        }
    }
}

std::set<int> RegionMaker::getPossibleMove(int area)
{
    std::set<int> moves;
    int myRegion = area2Region[area];

    // Check if moving area out of myRegion satisfy controls
    for (int i=0; i<controls.size(); ++i) {
        if (controls[i].CheckRemove(area, region2Area[myRegion]) == false) {
            return moves;
        }
    }

    const std::vector<long>& nn  = w[area].GetNbrs();
    for (int i=0; i<nn.size(); ++i) {
        // check neighbors, which region it belongs to
        int nbrRegion = area2Region[(int)nn[i]];
        if (nbrRegion != myRegion) {
            // check if moving area to nbrRegion satisfy controls
            for (int i=0; i<controls.size(); ++i) {
                if (controls[i].CheckAdd(area, region2Area[nbrRegion]) == false) {
                    return std::set<int>();
                }
            }
            moves.insert(nbrRegion);
        }
    }
    return moves;
}

void AZP::LocalImproving()
{
    // NOTE: this could be parallelized: when one thread update region A
    // and A exchanges area with another region B, only the threads interact
    // with region B should wait and other threads can run parallely to update
    // regions C,D,E... However, the mutex lock should be carefully designed...

    int improve = 1;
    std::set<int>::iterator it;
    //std::vector<int> rand_test = {7,4,3,5,3,3,1,1,0,3,3,2,3,4,2,0,0,0};
    //std::vector<int> rand_test1 = {57, 56, 52, 24, 16, 10, 3, 24, 51, 57, 22, 57, 46, 11, 46, 52, 50, 46, 10, 52, 24, 57, 3, 51, 7, 5, 20, 30, 28, 8, 26, 39, 43, 18, 55, 41, 36, 29, 17, 0, 56, 33, 35, 1, 23, 9, 32, 22, 2, 49, 15, 11, 48, 14, 16, 50, 34, 12, 42, 40, 31, 45, 44, 31, 30, 28, 8, 20, 40, 42, 17, 41, 18, 26, 55, 43, 39, 29, 36, 44, 31, 14, 11, 16, 2, 48, 0, 1, 15, 35, 50, 12, 23, 9, 49, 33, 32, 34, 56, 22, 24, 7, 45, 57, 10, 51, 5, 3, 46, 52};
    //int rr = 0, rr1 = 0;
    while (improve == 1) {
        std::vector<int> regions(p);
        for (int i=0; i<p; ++i) regions[i] = i;
        
        while (regions.size() > 0) {
            // step 3
            int randomRegion = 0;
            if (regions.size() > 1) {
                randomRegion = rng.nextInt((int)regions.size());
            }
            //randomRegion = rand_test[rr++];
            int region = regions[randomRegion];
            regions.erase(std::find(regions.begin(), regions.end(), region));

            // step 4
            // get bordering areas in region
            getBorderingAreas(region);

            boost::unordered_map<int, bool>& areas = region2Area[region];
            boost::unordered_map<int, bool>::iterator area_it;
            std::set<int>::iterator move_it;
            improve = 0;
            
            while (areas.size() > 1) {
                // step 5
                bool nothing_can_do = true;
                for (area_it = areas.begin(); area_it != areas.end(); ++area_it) {
                    if (area_it->second == true) {
                        nothing_can_do = false;
                        // pick a random bordering area, mark it as processed
                        int randomArea = area_it->first;
                        areas[randomArea] = false;
                        // get possible move of this randomArea
                        std::set<int> possibleMove = getPossibleMove(randomArea);
                        // check obj change before contiguity check
                        for (move_it = possibleMove.begin(); move_it != possibleMove.end(); ++move_it) {
                            int move = *move_it;
                            // request lock for region[move], pause threads[move]
                            bool valid = objective_function->TrySwap(randomArea, region, move);
                            if (valid) {
                                this->area2Region[randomArea] = move;
                                this->objInfo = objective_function->GetValue();
                                improve = 1;
                                // move happens, update bordering area
                                getBorderingAreas(region);
                                break;
                            }
                            // release lock for region[move], awake threads[move]
                        }
                        break; // don't continue;
                    }
                }
                if (nothing_can_do) {
                    break;
                }
            }
        }
    }
}



void AZPSA::LocalImproving()
{
    /*
    // Openshaw's Simulated Annealing for AZP algorithm
    int totalMoves = 0;
    int acceptedMoves = 0;
    double bestOBJ = this->objInfo;
    double currentOBJ = this->objInfo;
    std::vector<int> bestRegions = this->returnRegions();
    std::vector<int> currentRegions = this->returnRegions();
    boost::unordered_map<int, std::set<int> > region2AreaBest = this->region2Area;
    boost::unordered_map<int, int> area2RegionBest = this->area2Region;
    std::set<int>::iterator it;
    
    int improve = 1;
    while (improve == 1) {
        std::vector<int> regions(p);
        for (int i=0; i<p; ++i) regions[i] = i;

        while (regions.size() > 0) {
            // step 3
            int randomRegion = 0;
            if (regions.size() > 1) {
                randomRegion = rng.nextInt((int)regions.size());
            }
            //randomRegion = rand_test[rr++];
            int region = regions[randomRegion];
            regions.erase(std::find(regions.begin(), regions.end(), region));

            // step 4
            const std::set<int>& ba = this->returnBorderingAreas(region);
            const std::set<int>& aa = this->returnRegion2Area(region);
            std::set<int> borderingAreas = this->intersects(ba, aa);

            improve = 0;
            while (borderingAreas.size() > 0) {
                // step 5
                int randomArea = rng.nextInt((int)borderingAreas.size());
                it = borderingAreas.begin();
                std::advance(it, randomArea);
                int area = *it;
                //area = rand_test1[rr1++];
                borderingAreas.erase(area);
                std::set<int> posibleMove = intraBorderingAreas[area];

                int f = 0;
                if (this->region2Area[region].size() >= 2)  {
                    f = this->checkFeasibility(region, area, region2Area);
                }
                if (f == 1) {
                    for (it = posibleMove.begin(); it != posibleMove.end(); ++it) {
                        int move = *it;
                        this->swapArea(area, move, region2Area, area2Region);
                        double obj = this->recalcObj(region2Area, false);
                        this->swapArea(area, region, region2Area, area2Region);
                        if (obj <= bestOBJ) {
                            this->moveArea(area, move);
                            improve = 1;
                            this->objInfo = obj;
                            bestOBJ = obj;
                            bestRegions = this->returnRegions();
                            currentRegions = this->returnRegions();
                            region2AreaBest = this->region2Area;
                            area2RegionBest = this->area2Region;

                            // std::cout << "--- Local improvement (area, region)" << area << "," << move << std::endl;
                            // std::cout << "--- New Objective Function value: " << obj << std::endl;
                            // step 4

                            borderingAreas = intersects(returnBorderingAreas(region),
                                                        returnRegion2Area(region));
                            break;
                        } else {
                            double random = rng.nextDouble();
                            totalMoves += 1;
                            double sa = std::exp(-(obj - currentOBJ) / (currentOBJ * temperature));
                            if (sa > random) {
                                acceptedMoves += 1;
                                this->moveArea(area, move);
                                this->objInfo = obj;
                                currentOBJ = obj;
                                currentRegions = this->returnRegions();

                                // std::cout << "--- NON-Local improvement (area, region)" << area << "," << move << std::endl;
                                // std::cout << "--- New Objective Function value: " << obj << std::endl;
                                // step 4

                                borderingAreas = intersects(returnBorderingAreas(region),
                                                            returnRegion2Area(region));
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    this->objInfo = bestOBJ;
    this->region2Area = region2AreaBest;
    this->area2Region = area2RegionBest;
     */
}

void AZPTabu::LocalImproving()
{
    /*
    // Tabu search algorithm for Openshaws AZP-tabu (1995)
    double aspireOBJ = this->objInfo;
    double currentOBJ = this->objInfo;
    std::vector<int> aspireRegions = this->returnRegions();
    boost::unordered_map<int, std::set<int> > region2AreaAspire = this->region2Area;
    boost::unordered_map<int, int> area2RegionAspire = this->area2Region;
    std::vector<int> currentRegions = aspireRegions;
    std::vector<std::pair<int, int> > tabuList(tabuLength, std::make_pair(-1,-1));

    int c = 1;
    double epsilon = 1e-10;

    boost::unordered_map<std::pair<int, int>, double>::iterator mit;
    double minval;
    std::pair<int, int> move;

    while (c < convTabu) {
        // std::cout << "regions: ";
        this->objDict = this->makeObjDict();
        this->allCandidates();
        if (this->neighSolutions.size() == 0) {
            c += convTabu;
        } else {
            std::pair<int, int> moveNoTabu;
            int minFound = 0;
            boost::unordered_map<std::pair<int, int>, double> neighSolutionsCopy = neighSolutions;
            c += 1;
            move = find_notabu_move(neighSolutionsCopy, tabuList);
            if (move.first >= 0) {
                double obj4Move = this->neighSolutions[move];
                moveNoTabu = move;
                if (currentOBJ - obj4Move >= epsilon) {
                    minFound = 1;
                } else {
                    move = find_tabu_move(neighSolutionsCopy, tabuList);
                    if (move.first >= 0) {
                        double obj4Move = this->neighSolutions[move];
                        if  (aspireOBJ - obj4Move > epsilon) {
                            minFound = 1;
                        }
                    }
                }
            }
            if (minFound == 1) {
                int area = move.first;
                int region = move.second;
                double obj4Move = this->neighSolutions[move];
                int oldRegion = this->area2Region[area];
                this->updateTabuList(area, oldRegion, tabuList, tabuLength);
                this->moveArea(area, region);
                this->objInfo = obj4Move;
                if (aspireOBJ - obj4Move > epsilon) {
                    aspireOBJ = obj4Move;
                    aspireRegions = this->returnRegions();
                    region2AreaAspire = this->region2Area;
                    area2RegionAspire = this->area2Region;
                    c = 1;
                }
                currentOBJ = obj4Move;
                currentRegions = this->returnRegions();
            } else {
                move = moveNoTabu;
                int area = move.first;
                int region = move.second;
                double obj4Move = this->neighSolutions[move];
                int oldRegion = this->area2Region[area];
                this->updateTabuList(area, oldRegion, tabuList, tabuLength);
                this->moveArea(area, region);
                this->objInfo = obj4Move;
                currentOBJ = obj4Move;
                currentRegions = this->returnRegions();
            }
        }
    }
    this->objInfo = aspireOBJ;
    this->regions = aspireRegions;
    this->region2Area = region2AreaAspire;
    this->area2Region = area2RegionAspire;
     */
}

std::vector<std::pair<int, int> > AZPTabu::updateTabuList(int area, int region, std::vector<std::pair<int, int> >& aList, int endInd)
{
    // Add a new value to the tabu list.
    // always added to the front and remove the last one
    aList.insert(aList.begin(), std::make_pair(area, region));
    aList.pop_back();

    return aList;
}

boost::unordered_map<int, double> AZPTabu::makeObjDict()
{
    // constructs a dictionary with the objective function per region
    boost::unordered_map<int, double> objDict;

    /*
    boost::unordered_map<int, std::set<int> >::iterator it;
    std::set<int>::iterator sit;

    for (it = region2Area.begin(); it != region2Area.end(); ++it) {
        int region = it->first;

        const std::set<int>& areasIdsIn = region2Area[region];
        // compute centroid of this set of areas
        double* dataAvg = new double[m];

        for (sit = areasIdsIn.begin(); sit != areasIdsIn.end(); ++sit) {
            int idx = *sit;
            for (int j=0; j<m; ++j) {
                dataAvg[j] += data[idx][j];
            }
        }
        double n_areas = (double)areasIdsIn.size();
        for (int j=0; j<m; ++j) {
            dataAvg[j] /= n_areas;
        }

        // distance from area in this region to centroid
        objDict[region]  = 0.0;
        for (sit = areasIdsIn.begin(); sit != areasIdsIn.end(); ++sit) {
            int idx = *sit;
            double dist = DataUtils::EuclideanDistance(data[idx], dataAvg, m, NULL);
            objDict[region] += dist;
        }

        delete[] dataAvg;
    }
     */
    return objDict;
}

void AZPTabu::allCandidates()
{
    /*
    // Select neighboring solutions.
    boost::unordered_map<int, std::set<int> > intraCopy = intraBorderingAreas;
    boost::unordered_map<int, std::set<int> > region2AreaCopy = region2Area;
    boost::unordered_map<int, int> area2RegionCopy = area2Region;

    neighSolutions.clear();
    std::set<int>::iterator rit;
    boost::unordered_map<int, std::set<int> >::iterator it;

    for (it = intraCopy.begin(); it!= intraCopy.end(); ++it) {
        int area = it->first;
        int regionIn = this->area2Region[area];
        std::set<int> regions4Move = this->intraBorderingAreas[area];
        if (this->region2Area[regionIn].size() > 1) {
            for (rit = regions4Move.begin(); rit != regions4Move.end(); ++rit) {
                int region = *rit;
                int f = this->checkFeasibility(regionIn, area, this->region2Area);
                if (f == 1) {
                    this->swapArea(area, region, region2AreaCopy, area2RegionCopy);
                    std::pair<int, int> modifiedRegions = std::make_pair(region, regionIn);
                    double obj = this->recalcObj(region2AreaCopy, modifiedRegions);
                    this->neighSolutions[std::make_pair(area, region)] = obj;
                    this->swapArea(area, regionIn, region2AreaCopy, area2RegionCopy);
                }
            }
        }
    }
     */
}
