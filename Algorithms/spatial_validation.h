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
    double entropy;
    double simpson;
    double min_cluster_size;
    double max_cluster_size;
    double mean_cluster_size;
    bool is_spatially_contiguous;
    Fragmentation() : entropy(0), simpson(0), min_cluster_size(0),
    max_cluster_size(0), mean_cluster_size(0), is_spatially_contiguous(true) {}
};

struct Compactness {
    double isoperimeter_quotient;
    double area;
    double perimeter;
    Compactness() : isoperimeter_quotient(0), area(0), perimeter(0) {}
};

struct Diameter {
    int steps;
    double ratio;
    Diameter() : steps(0), ratio(0) {}
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
};

class SpatialValidationCluster
{
public:
    SpatialValidationCluster(int cid, const std::vector<int>& elements, GeoDaWeight* weights,
                             std::map<int, int>& cluster_dict,
                             std::vector<OGRGeometry*>& geoms,
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
    std::vector<OGRGeometry*>& geoms;
    Shapefile::ShapeType shape_type;
};



class SpatialValidation
{
public:
    SpatialValidation(int num_obs, const std::vector<std::vector<int> >& clusters,
                      GeoDaWeight* weights, std::vector<OGRGeometry*>& geoms,
                      Shapefile::ShapeType shape_type);
	virtual ~SpatialValidation();
    
    void Run();
    
    std::vector<std::vector<int> > GetClusters();
    
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
    std::vector<OGRGeometry*> geoms;
    Shapefile::ShapeType shape_type;
    
    Fragmentation fragmentation;
    std::vector<Fragmentation> fragmentations;
    std::vector<Compactness> compactnesses;
    std::vector<Diameter> diameters;
};

#endif
