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

bool ZoneControl::CheckZone(const std::vector<int>& candidates)
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

double AreaManager::getDistance2Region(int area, const std::set<int>& areaList)
{
    std::vector<double> d(m,0);
    for (int i=0; i<m; ++i) d[i] = data[area][i];
    double dist;

    // get centroid from a areaList (region)
    if (cache_cent.find(areaList) != cache_cent.end()) {
        dist = DataUtils::EuclideanDistance(d, cache_cent[areaList]);
    } else {
        std::vector<double> centroidRegion(m, 0);
        std::set<int>::iterator it;
        for (it = areaList.begin(); it != areaList.end(); ++it) {
            int area_id = *it;
            for (int j=0; j<m; ++j) {
                centroidRegion[j] += data[area_id][j];
            }
        }
        for (int j=0; j<m; ++j) {
            centroidRegion[j] /= (double)areaList.size();
        }

        // get distance between area and region
        dist = DataUtils::EuclideanDistance(d, centroidRegion);

        // cache it
        cache_cent[areaList] = centroidRegion;
    }
    return dist;
}


////////////////////////////////////////////////////////////////////////////////
////// RegionMaker
////////////////////////////////////////////////////////////////////////////////
RegionMaker::RegionMaker(int _p, GalElement* const _w,
                         double** _data, // row-wise
                         RawDistMatrix* _dist_matrix,
                         int _n, int _m)
: p(_p), w(_w), data(_data), dist_matrix(_dist_matrix), n(_n), m(_m),
am(_n, _m, _w, _data, _dist_matrix), objInfo(-1)
{
    // init unassigned areas
    for (int i=0; i<n; ++i) {
        unassignedAreas[i] = true;
        std::set<int> nbrs;
        const std::vector<long>& nn = w[i].GetNbrs();
        for (int i=0; i<nn.size(); ++i)  {
            nbrs.insert((int)nn[i]);
        }
        nbrSet.push_back(nbrs);
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

    // compute objective function
    objInfo = this->getObj();

    //  Gets the intrabordering areas for local improvement later
    this->getIntraBorderingAreas();
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

    //self.changedRegion = 'null'
    //self.newExternal = self.potentialRegions4Area.keys()
    this->changedRegion = -1;
    this->newExternal.clear();
    boost::unordered_map<int, std::set<int> >::iterator it;
    for (it = potentialRegions4Area.begin(); it != potentialRegions4Area.end(); ++it) {
        this->newExternal[it->first] = true;
    }
}

void RegionMaker::assignAreaStep1(int areaID, int regionID)
{
    //  Assgin an area to a region
    region2Area[regionID].insert(areaID);

    // attach region with area
    area2Region[areaID] = regionID;

    // remove areaID from unassignedAreas
    unassignedAreas.erase(areaID);

    // add areaID to assignedAreas
    assignedAreas[areaID] = true;

    // update current externalNeighs by adding neighbors of current area
    // remove any assigned area (to a region) from externalNeighs
    this->oldExternal = this->externalNeighs;

    //self.externalNeighs = (self.externalNeighs | setNeighs) - setAssigned
    std::vector<long> neighs = this->w[areaID].GetNbrs();
    for (int i=0; i< neighs.size(); ++i) {
        int nn = (int)neighs[i];
        this->externalNeighs[nn] = true;
    }
    boost::unordered_map<int, bool>::iterator it;
    for (it = assignedAreas.begin(); it != assignedAreas.end(); ++it){
        this->externalNeighs.erase(it->first);
    }

    // mark which are new ones in externalNeighs
    //self.newExternal = self.externalNeighs - self.oldExternal
    this->newExternal = this->externalNeighs;
    for (it = oldExternal.begin(); it != oldExternal.end(); ++it) {
        this->newExternal.erase(it->first);
    }
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
        std::set<int> regionIDs = it->second;
        //if (regionIDs.size() == 1) {
            // there is no need to compute distance since its only one option(region)
        //    int region = *regionIDs.begin();
        //    candidateInfo[std::make_pair(areaID, region)] = 0;
        //} else {
            // compute distance from areaID to different regions
            for (rit = regionIDs.begin(); rit != regionIDs.end(); ++rit) {
                int region = *rit;
                if (region != changedRegion && newExternal.find(areaID) == newExternal.end()) {
                    // there is no need to recompute the distance since there is
                    // no change in the region
                    lastRegion = region;
                } else {
                    std::pair<int, int> a_r = std::make_pair(areaID, region);
                    std::set<int>& areasIdsIn = region2Area[region];
                    double regionDistance = am.getDistance2Region(areaID, areasIdsIn);
                    // (areaID, region): distance
                    candidateInfo[a_r] = regionDistance;
                }
            }
        //}
    }

    if (candidateInfo.empty())  {
        this->changedRegion = lastRegion;

    } else {

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

        // assign select areaID to regionID, and process the neighbors of areaID
        this->assignArea(aid, rid);
    }
}

void RegionMaker::assignArea(int areaID, int regionID)
{
    //Assign an area to a region and updates potential regions for its neighs

    this->changedRegion = regionID; // mark this region has been changed
    this->addedArea = areaID;

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


double RegionMaker::getObj()
{
    //Return the value of the objective function
    if (this->objInfo < 0) {
        this->calcObj();
    }
    return this->objInfo;
}

void RegionMaker::calcObj()
{
    // Calculate the value of the objective function,
    this->objInfo = this->getObjective(this->region2Area);
}

double RegionMaker::recalcObj(boost::unordered_map<int, std::set<int> >& region2AreaDict, bool use_cache)
{
    // Re-calculate the value of the objective function
    // could use memory cached results
    double obj = 0.0;
    if (objDict.empty() || use_cache==false) {
        obj = this->getObjective(region2AreaDict);

    } else {
        //obj = this->getObjectiveFast(region2AreaDict);
        obj = 0.0;
        boost::unordered_map<int, std::set<int> >::iterator it;
        for (it = region2AreaDict.begin(); it != region2AreaDict.end(); ++it) {
            int region = it->first;
            obj += objDict[region];
        }
    }
    return obj;
}

double RegionMaker::recalcObj(boost::unordered_map<int, std::set<int> >& region2AreaDict, std::pair<int, int>& modifiedRegions)
{
    // Re-calculate the value of the objective function
    double obj;
    if (objDict.empty()) {
        obj = this->getObjective(region2AreaDict);
    } else {
        obj = this->getObjectiveFast(region2AreaDict, modifiedRegions);
    }
    return obj;
}

double RegionMaker::getObjective(boost::unordered_map<int, std::set<int> >& region2AreaDict)
{
    // Calculate the value of the objective function
    // self.objInfo = self.getObjective(self.region2Area)
    // Return the value of the objective function from regions to area dictionary
    // Sum of squares from each area to the region's centroid
    //boost::unordered_map<int, double> objDist;
    objDict.clear(); // clean cache since recalcuation

    boost::unordered_map<int, std::set<int> >::iterator it;
    std::set<int>::iterator sit;

    for (it = region2AreaDict.begin(); it != region2AreaDict.end(); ++it) {
        int region = it->first;
        std::set<int>& areasIdsIn = it->second;
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

    double ss = 0;
    boost::unordered_map<int, double>::iterator mid;
    for (mid = objDict.begin(); mid != objDict.end(); ++mid) {
        ss += mid->second;
    }
    return ss;
}


double RegionMaker::getObjectiveFast(boost::unordered_map<int, std::set<int> >& region2AreaDict,
                                     std::pair<int, int>& modifiedRegions)
{
    //Return the value of the objective function from regions to area dictionary
    double obj = 0.0;
    boost::unordered_map<int, std::set<int> >::iterator it;
    std::set<int>::iterator sit;

    for (it = region2AreaDict.begin(); it != region2AreaDict.end(); ++it) {
        int region = it->first;
        if (region == modifiedRegions.first || region == modifiedRegions.second) {
            double valRegion = 0.0;
            const std::set<int>& areasIdsIn = region2AreaDict[region];
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
            for (sit = areasIdsIn.begin(); sit != areasIdsIn.end(); ++sit) {
                int idx = *sit;
                double dist = DataUtils::EuclideanDistance(data[idx], dataAvg, m, NULL);
                valRegion += dist;
            }
            obj += valRegion;
        } else {
            obj += objDict[region];
        }
    }

    return obj;
}

void RegionMaker::getIntraBorderingAreas()
{
    std::set<int>::iterator it;
    for (int rid=0; rid<p; ++rid) {
        // for each region, get all areas
        const std::set<int>& areas2Evl = this->region2Area[rid];
        // then get all neighbors of areas2Eval
        boost::unordered_map<int, bool> neighsNoRegion;
        for (it = areas2Evl.begin(); it !=areas2Evl.end(); ++it) {
            int area = *it;
            const std::vector<long>& nn = w[area].GetNbrs();
            for (int j=0; j<nn.size(); ++j) {
                neighsNoRegion[nn[j]] = true;
            }
        }
        // remove(mark) areas already in region
        for (it = areas2Evl.begin(); it !=areas2Evl.end(); ++it) {
            int area = *it;
            neighsNoRegion[area] = false;
        }
        // save
        boost::unordered_map<int, bool>::iterator mit;
        for (mit = neighsNoRegion.begin(); mit != neighsNoRegion.end(); ++mit) {
            int neigh = mit->first;
            if (mit->second == true) {
                this->intraBorderingAreas[neigh].insert(rid);
            }
        }
    }
}

std::set<int> RegionMaker::returnRegion2Area(int regionID)
{
    return region2Area[regionID];
}

std::set<int> RegionMaker::returnBorderingAreas(int regionID)
{
    // slow
    std::set<int> areas2Eval = this->returnRegion2Area(regionID);
    std::set<int> borderingAreas;
    std::set<int>::iterator it;
    for (it = areas2Eval.begin(); it != areas2Eval.end(); ++it) {
        int area = *it;
        if (intraBorderingAreas.find(area) != intraBorderingAreas.end() &&
            intraBorderingAreas[area].empty() == false) {
            borderingAreas.insert(area);
        }
    }
    return borderingAreas;
}

std::set<int> RegionMaker::unions(const std::set<int>& s1, const std::set<int>& s2)
{
    std::set<int> s_all;
    std::set<int>::iterator it;
    for (it = s1.begin(); it != s1.end(); ++it) {
        s_all.insert(*it);
    }
    for (it = s2.begin(); it != s2.end(); ++it) {
        s_all.insert(*it);
    }
    return s_all;
}

std::set<int> RegionMaker::intersects(const std::set<int>& s1, const std::set<int>& s2)
{
    std::set<int> s_inter;
    std::set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(),
                          std::inserter(s_inter, s_inter.begin()));
    return s_inter;
}

std::set<int> RegionMaker::removes(const std::set<int>& s1, const std::set<int>& s2)
{
    std::set<int> s_res =  s1;
    std::set<int>::iterator it;
    for (it = s2.begin(); it != s2.end(); ++it) {
        s_res.erase(*it);
    }
    return s_res;
}

int RegionMaker::checkFeasibility(int regionID, int areaID,
                                  boost::unordered_map<int, std::set<int> >& region2AreaDict)
{
    // Check feasibility from a change region (remove an area from a region)
    std::set<int> areas2Eval = region2AreaDict[regionID];
    areas2Eval.erase(areaID);

    // remove the area first
    int seedArea = *areas2Eval.begin();
    // then, start from 1st object, do BFS
    std::stack<int> processed_ids;
    processed_ids.push(seedArea);
    while (processed_ids.empty() == false) {
        int fid = processed_ids.top();
        processed_ids.pop();
        areas2Eval.erase(fid); // remove area from current group as processed
        const std::vector<long>& nbrs = w[fid].GetNbrs();
        for (int i=0; i<nbrs.size(); i++ ) {
            int nid = nbrs[i];
            if (areas2Eval.find(nid) != areas2Eval.end()) {
                // only processed the neighbor in current group
                processed_ids.push(nid);
            }
        }
    }
    // all should be removed if all connected
    return areas2Eval.empty();
    /*
    int feasible = 0;


    // newRegion = (set([seedArea]) | set(self.areas[seedArea].neighs)) & set(areas2Eval)
    std::set<int> seedAndNeighs, newRegion, newAdded, newNeighs;
    
    seedAndNeighs = this->nbrSet[seedArea];
    seedAndNeighs.insert(seedArea);
    
    newRegion = intersects(seedAndNeighs, areas2Eval);
    
    areas2Eval.erase(seedArea);
    int flag = 1;
    // newAdded = newRegion - set([seedArea])
    newAdded = newRegion;
    newAdded.erase(seedArea);
    std::set<int>::iterator it;
    while (flag == 1) {
        // for area in newAdded:
        for (it = newAdded.begin(); it != newAdded.end(); ++it) {
            int area = *it;
            // newNeighs = newNeighs | (((set(self.areas[area].neighs) &set(region2AreaDict[regionID])) - set([areaID])) - newRegion)
            std::set<int> ar = intersects(nbrSet[area], region2AreaDict[regionID]);
            ar.erase(areaID);
            ar = removes(ar, newRegion);
            newNeighs = unions(newNeighs, ar);
            areas2Eval.erase(area);
        }
        newNeighs = removes(newNeighs, newAdded);
        newAdded = newNeighs;
        newRegion = unions(newRegion, newAdded);
        if (areas2Eval.empty()) {
            feasible = 1;
            flag = 0;
            break;
        } else if (newNeighs.empty() && areas2Eval.size() > 0) {
            feasible = 0;
            flag = 0;
            break;
        }
    }
    return feasible;
     */
}

void RegionMaker::swapArea(int area, int newRegion, boost::unordered_map<int, std::set<int> >& region2AreaDict, boost::unordered_map<int, int>& area2RegionDict)
{
    // Removed an area from a region and appended it to another one
    int oldRegion = area2RegionDict[area];
    
    region2AreaDict[oldRegion].erase(area);
    region2AreaDict[newRegion].insert(area);
    
    area2RegionDict[area] = newRegion;
}

void RegionMaker::moveArea(int areaID, int regionID)
{
    // Move an area to a region
    int oldRegion = this->area2Region[areaID];
    this->region2Area[oldRegion].erase(areaID);
    this->region2Area[regionID].insert(areaID);
    this->area2Region[areaID] = regionID;

    // update intraBorderingAreas: slow
    std::set<int> toUpdate = this->nbrSet[areaID];
    toUpdate.insert(areaID);
    
    std::set<int>::iterator it;
    for (it = toUpdate.begin(); it != toUpdate.end(); ++it) {
        int area = *it;
        int regionIn = this->area2Region[area];
        const std::set<int>& areasIdsIn = this->region2Area[regionIn];
        const std::set<int>& aNeighs = this->nbrSet[area];
        std::set<int> neighsInOther = removes(aNeighs, areasIdsIn);
        // if len(neighsInOther) == 0 and area in self.intraBorderingAreas:
        if (neighsInOther.empty() &&
            intraBorderingAreas.find(area) != intraBorderingAreas.end()) {
            intraBorderingAreas.erase(area);
        } else {
            std::set<int> borderRegions;
            std::set<int>::iterator bit;
            for (bit = neighsInOther.begin(); bit != neighsInOther.end(); ++bit) {
                int neigh = *bit;
                borderRegions.insert(area2Region[neigh]);
            }
            if (intraBorderingAreas.find(area) != intraBorderingAreas.end()) {
                intraBorderingAreas.erase(area);
            }
            intraBorderingAreas[area] = borderRegions;
        }
    }
    //this->calcObj();
}

void AZP::LocalImproving()
{
    int improve = 1;
    std::set<int>::iterator it;
    //std::vector<int> rand_test = {7,4,3,5,3,3,1,1,0,3,3,2,3,4,2,0,0,0};
    //std::vector<int> rand_test1 = {57, 56, 52, 24, 16, 10, 3, 24, 51, 57, 22, 57, 46, 11, 46, 52, 50, 46, 10, 52, 24, 57, 3, 51, 7, 5, 20, 30, 28, 8, 26, 39, 43, 18, 55, 41, 36, 29, 17, 0, 56, 33, 35, 1, 23, 9, 32, 22, 2, 49, 15, 11, 48, 14, 16, 50, 34, 12, 42, 40, 31, 45, 44, 31, 30, 28, 8, 20, 40, 42, 17, 41, 18, 26, 55, 43, 39, 29, 36, 44, 31, 14, 11, 16, 2, 48, 0, 1, 15, 35, 50, 12, 23, 9, 49, 33, 32, 34, 56, 22, 24, 7, 45, 57, 10, 51, 5, 3, 46, 52};
    int rr = 0, rr1 = 0;
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
            //borderingAreas = list(set(self.returnBorderingAreas(region)) & set(self.returnRegion2Area(region)))
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
                const std::set<int>& posibleMove = intraBorderingAreas[area];

                // todo: get possible move: nbrsNotInRegion

                // check obj change before contiguity check

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
                        if (obj <= this->objInfo) {
                            this->moveArea(area, move);
                            this->objInfo = obj;
                            improve = 1;
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

void getBorderingAreas()
{
    // check every area, and return who has neighbors out of the region
}

void AZPSA::LocalImproving()
{
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
}

void AZPTabu::LocalImproving()
{
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
    return objDict;
}

void AZPTabu::allCandidates()
{
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
}
