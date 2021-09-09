/**
 * GeoDa TM, Copyright (C) 2011-2015 by Luc Anselin - all rights reserved
 *
 * This file is part of GeoDa.
 *
 * GeoDa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GeoDa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Created: 8/9/2021 lixun910@gmail.com
 */

#ifndef __GEODA_CENTER_SPATIAL_KMEANS_H__
#define __GEODA_CENTER_SPATIAL_KMEANS_H__

#include <vector>
#include <map>

class GeoDaWeight;
class SpatialKMeansCluster;
class SpatialKMeans;

class SpatialKMeansComponent
{
public:
    SpatialKMeansComponent(int cid, const std::vector<int>& elements,
                           GeoDaWeight* weights, std::map<int, int>& cluster_dict);
    virtual ~SpatialKMeansComponent();
    
    int GetClusterId() { return cid; }
    void SetClusterId(int cid) { this->cid = cid; }
    
    int GetSize() { return (int)elements.size(); }
    std::vector<int> GetElements() { return elements; }
    
    void Merge(SpatialKMeansComponent* comp);
    
    bool Has(int eid);
    
    bool isIsland;
    bool isSingleton;
    bool isSurroundedSingleton;
    
protected:
    GeoDaWeight* weights;
    std::map<int, int>& cluster_dict;
    std::vector<int> elements;
    int cid;
    std::map<int, bool> elements_dict;
};

class SpatialKMeansCluster
{
public:
    SpatialKMeansCluster(int cid, const std::vector<int>& elements, GeoDaWeight* weights,
                         std::map<int, int>& cluster_dict);
    virtual ~SpatialKMeansCluster();
    
    // find singletons that only connect to one cluster
    std::vector<SpatialKMeansComponent*> GetSurroundedSingletons();
    
    std::vector<SpatialKMeansComponent*> GetComponentsBySize(int component_size);
    
    void MergeComponent(SpatialKMeansComponent* from, SpatialKMeansComponent* to);
    
    void RemoveComponent(SpatialKMeansComponent* comp);
    
    std::vector<int> GetCoreElements();
    
    int GetCoreSize();
    
    int GetComponentSize(int eid);
    
    int GetSmallestComponentSize();
    
    SpatialKMeansComponent* GetComponent(int eid);
    
    bool BelongsToCore(int eid);

    std::vector<int> GetComponentSize();
    
protected:
    GeoDaWeight* weights;
    std::map<int, int>& cluster_dict;
    std::vector<int> elements;
    int cid;
    SpatialKMeansComponent* core;
    std::vector<SpatialKMeansComponent*> components;
    std::map<int, SpatialKMeansComponent*> component_dict;
};

class SpatialKMeans 
{
public:
    SpatialKMeans(int num_obs, const std::vector<std::vector<int> >& clusters,
                  GeoDaWeight* weights);
	virtual ~SpatialKMeans();
    
    void Run();
    
    std::vector<std::vector<int> > GetClusters();
    
    bool IsValid() { return valid; }
    
    bool IsSpatiallyConstrained();
    
protected:
    void UpdateComponent(SpatialKMeansComponent* moved_comp,
                         SpatialKMeansComponent* target);
    
    void MoveComponent(SpatialKMeansComponent* comp);
    
    int GetSmallestComponentSize();
    
    std::vector<SpatialKMeansCluster*> GetClustersByComponentSize(int sz);
    
protected:
    GeoDaWeight* weights;
    std::vector<std::vector<int> > clusters;
    int num_obs;
    int num_clusters;
    
    std::map<int, int> cluster_dict;
    bool valid;
    
    std::vector<SpatialKMeansCluster*> sk_clusters;
};

#endif
