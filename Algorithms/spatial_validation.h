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

#ifndef __GEODA_CENTER_SPATIAL_VALIDATION_H__
#define __GEODA_CENTER_SPATIAL_VALIDATION_H__

#include <vector>
#include <map>
#include <ogrsf_frmts.h>

#include "../ShpFile.h"

class GeoDaWeight;
class SpatialValidationCluster;
class SpatialValidation;

struct Fragmentation {
    int n;
    double entropy;
    double max_entropy;
    double simpson;
    int min_cluster_size;
    int max_cluster_size;
    double mean_cluster_size;
    bool is_spatially_contiguous;
    
    Fragmentation() : n(0), entropy(0), max_entropy(0), simpson(0), min_cluster_size(0),
    max_cluster_size(0), mean_cluster_size(0), is_spatially_contiguous(true) {}
    
    Fragmentation& operator = (const Fragmentation& item) {
        n = item.n;
        entropy = item.entropy;
        max_entropy = item.max_entropy;
        simpson = item.simpson;
        min_cluster_size = item.min_cluster_size;
        max_cluster_size = item.max_cluster_size;
        mean_cluster_size = item.mean_cluster_size;
        is_spatially_contiguous = item.is_spatially_contiguous;
        return *this;
    }
};

struct Compactness {
    double isoperimeter_quotient;
    double area;
    double perimeter;
    Compactness() : isoperimeter_quotient(0), area(0), perimeter(0) {}
    
    Compactness& operator = (const Compactness& item) {
        isoperimeter_quotient = item.isoperimeter_quotient;
        area = item.area;
        perimeter = item.perimeter;
        return *this;
    }
};

struct Diameter {
    int steps;
    double ratio;
    Diameter() : steps(0), ratio(0) {}
    
    Diameter& operator = (const Diameter& item) {
        steps = item.steps;
        ratio = item.ratio;
        return *this;
    }
};

class SpatialValidationComponent
{
public:
    SpatialValidationComponent(int cid, const std::vector<int>& elements,
                               GeoDaWeight* weights, std::map<int, int>& cluster_dict,
                               std::map<int, std::vector<int> >& edges);
    virtual ~SpatialValidationComponent();
    
    int GetClusterId() { return cid; }
    void SetClusterId(int cid) { this->cid = cid; }
    
    int GetSize() { return (int)elements.size(); }
    std::vector<int> GetElements() { return elements; }
        
    bool Has(int eid);
    
    Diameter ComputeDiameter();
    
    bool isIsland;
    bool isSingleton;
    bool isSurroundedSingleton;
    
protected:
    GeoDaWeight* weights;
    std::map<int, int>& cluster_dict;
    std::vector<int> elements;
    int cid;
    std::map<int, bool> elements_dict;
    std::map<int, std::vector<int> > edges;
    
    struct Step {
        int eid;
        std::map<int, int>& steps;
        Step(int eid, std::map<int, int>& steps) : eid(eid), steps(steps){}
        bool operator < (const Step& item) const {
            return steps[eid] > steps[item.eid];
        }
        Step& operator = (const Step& item) {
            eid = item.eid;
            steps = item.steps;
            return *this;
        }
    };
    
    std::vector<int> shortest_paths;
    void ComputeDiameterThread(int start, int end);
};

class SpatialValidationCluster
{
public:
    SpatialValidationCluster(int cid, const std::vector<int>& elements, GeoDaWeight* weights,
                             std::map<int, int>& cluster_dict,
                             std::vector<Shapefile::RecordContents*>& geoms,
                             Shapefile::ShapeType shape_type);
    virtual ~SpatialValidationCluster();
    
    std::vector<int> GetCoreElements();
    
    int GetSize();
    
    int GetCoreSize();
    
    int GetComponentSize();
    
    SpatialValidationComponent* GetComponent(int eid);
        
    Fragmentation ComputeFragmentation();
    
    Compactness ComputeCompactness();
    
    Diameter ComputeDiameter();
    
protected:
    GeoDaWeight* weights;
    std::map<int, int>& cluster_dict;
    std::vector<int> elements;
    int cid;
    SpatialValidationComponent* core;
    std::vector<SpatialValidationComponent*> components;
    std::map<int, SpatialValidationComponent*> component_dict;
    std::vector<Shapefile::RecordContents*> geoms;
    Shapefile::ShapeType shape_type;
    
};



class SpatialValidation
{
public:
    SpatialValidation(int num_obs, const std::vector<std::vector<int> >& clusters,
                      GeoDaWeight* weights,
                      std::vector<Shapefile::RecordContents*>& geoms,
                      Shapefile::ShapeType shape_type);
	virtual ~SpatialValidation();
            
    bool IsValid() { return valid; }
    
    Fragmentation GetFragmentation() { return fragmentation; }
        
    std::vector<Fragmentation> GetFragmentationFromClusters() { return fragmentations; }
    
    std::vector<Compactness> GetCompactnessFromClusters() { return compactnesses; }
    
    std::vector<Diameter> GetDiameterFromClusters() { return diameters; }
        
    bool IsSpatiallyConstrained();
    
protected:
    void ComputeFragmentation();
    
    void ComputeCompactness();
    
    void ComputeDiameter();
    
protected:
    GeoDaWeight* weights;
    std::vector<std::vector<int> > clusters;
    int num_obs;
    int num_clusters;
    std::map<int, int> cluster_dict;
    bool valid;
    std::vector<SpatialValidationCluster*> sk_clusters;
    std::vector<Shapefile::RecordContents*> geoms;
    Shapefile::ShapeType shape_type;
    
    Fragmentation fragmentation;
    std::vector<Fragmentation> fragmentations;
    std::vector<Compactness> compactnesses;
    std::vector<Diameter> diameters;
};

#endif
