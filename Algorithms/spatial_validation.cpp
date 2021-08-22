#include <assert.h>
#include <algorithm>
#include <queue>
#include <stack>
#include <set>
#include <iostream>
#include <boost/unordered_map.hpp>

#include "../ShapeOperations/GeodaWeight.h"
#include "spatial_validation.h"

SpatialValidationComponent::SpatialValidationComponent(int cid, const std::vector<int>& elements,
                                                       GeoDaWeight* weights,
                                                       std::map<int, int>& cluster_dict,
                                                       std::map<int, std::vector<int> >& edges)
: cid(cid), elements(elements), weights(weights), cluster_dict(cluster_dict), edges(edges)
{
    int num_elements = (int)elements.size();
    isSingleton = num_elements == 1;
    isIsland = isSingleton && weights->GetNeighbors(elements[0]).empty();
    
    isSurroundedSingleton = false;
    if (isSingleton) {
        std::vector<long> nbrs = weights->GetNeighbors(elements[0]);
        boost::unordered_map<long ,bool> nbr_dict;
        for (int i = 0; i < (int)nbrs.size(); ++i) {
            if (elements[0] != (int)nbrs[i]) {
                nbr_dict[cluster_dict[(int)nbrs[i]]] = true;
            }
        }
        isSurroundedSingleton = nbr_dict.size() == 1;
    }
    
    for (int i = 0; i < num_elements; ++i) {
        elements_dict[elements[i]] = true;
    }
}

SpatialValidationComponent::~SpatialValidationComponent()
{
}

Diameter SpatialValidationComponent::ComputeDiameter()
{
    int diameter = 0;
    
    // simplified dijkstra
    int n = (int)elements.size();
    for (int i = 0; i < n; ++i) {
        int e = elements[i];
        
        // steps from e to others
        std::map<int, int> steps;
        for (int j = 0; j < n; ++j) {
            steps[elements[j]] = elements[j] == e ? 0 :INT_MAX;
        }
        
        std::map<int, bool> visited;
        
        std::vector<Step> cands;
        cands.push_back(Step(e, steps));
        
        while (visited.size() < n) {
            
            std::make_heap(cands.begin(), cands.end());
            std::pop_heap(cands.begin(), cands.end());
            
            Step item = cands.back();
            cands.pop_back();
            
            int tmpid = item.eid;
            visited[tmpid] = true;
            
            std::vector<int> nbrs = edges[tmpid];
            for (int j = 0; j < (int)nbrs.size(); ++j) {
                int nb = nbrs[j];
                
                    // steps from e -> nb
                    int new_steps = steps[tmpid] + 1;
                    if (new_steps < steps[nb]) {
                        steps[nb] = new_steps;
                        if (new_steps > diameter) {
                            diameter = new_steps;
                        }
                        // update heap
                        
                    }
                
                if (visited.find(nb) == visited.end()) {
                    cands.push_back(Step(nb, steps));
                }
            }
        }
        
        // max steps
    }
    
    Diameter diam;
    diam.steps = diameter;
    diam.ratio = diameter / (double) elements.size();
    
    return diam;
}

bool SpatialValidationComponent::Has(int eid)
{
    return elements_dict[eid];
}


SpatialValidationCluster::SpatialValidationCluster(int cid, const std::vector<int>& elements,
                                                   GeoDaWeight* weights,
                                                   std::map<int, int>& cluster_dict,
                                                   std::vector<OGRGeometry*>& geoms,
                                                   Shapefile::ShapeType shape_type)
: cid(cid), elements(elements), cluster_dict(cluster_dict), weights(weights),
core(0), geoms(geoms), shape_type(shape_type)
{
    int num_elements = (int)elements.size();
    
    // create components from elements
    std::map<int, bool> visited;
    for (int i = 0; i < num_elements; ++i) {
        int eid = elements[i];
        if (visited[eid]) {
            continue;
        }
        std::vector<int> component;
        visited[eid] = true;
        
        // find contiguous neighbors
        std::stack<int> stack;
        stack.push(eid);
        
        std::map<int, std::vector<int> > edges;
        
        while (!stack.empty()) {
            int tmp_id = stack.top();
            stack.pop();
            component.push_back(tmp_id);
            std::vector<long> nbrs = weights->GetNeighbors(tmp_id);
            for (int j = 0; j < (int)nbrs.size(); ++j) {
                int neighbor = (int)nbrs[j];
                if (cluster_dict[neighbor] == this->cid && !visited[neighbor]) {
                    visited[neighbor] = true;
                    stack.push(neighbor);
                    edges[tmp_id].push_back(neighbor);
                    edges[neighbor].push_back(tmp_id);
                }
            }
        }
        
        
        SpatialValidationComponent* c = new SpatialValidationComponent(this->cid, component, weights, cluster_dict, edges);
        components.push_back(c);
        
        // create lookup table
        for (int j = 0; j < (int)component.size(); ++j) {
            int eid = component[j];
            component_dict[eid] = c;
        }
        
        if (core == 0 || core->GetSize() < c->GetSize()) {
            core = c;
        }
    }
}

SpatialValidationCluster::~SpatialValidationCluster()
{
    // clean up components
    for (int i = 0; i < (int)components.size(); ++i) {
        delete components[i];
    }
}

SpatialValidationComponent* SpatialValidationCluster::GetComponent(int eid)
{
    // get component by giving an element id
    return component_dict[eid];
}

std::vector<int> SpatialValidationCluster::GetCoreElements()
{
    return core->GetElements();
}

int SpatialValidationCluster::GetComponentSize()
{
    return (int)components.size();
}

int SpatialValidationCluster::GetCoreSize()
{
    return core->GetSize();
}

int SpatialValidationCluster::GetSize()
{
    int sz = 0;
    for (int i = 0; i < (int)components.size(); ++i) {
        sz += components[i]->GetSize();
    }
    return sz;
}

Fragmentation SpatialValidationCluster::ComputeFragmentation()
{
    // for each cluster in terms of its contiguous subclusters
    Fragmentation frag;
    
    // list number of contiguous subclusters with size distribution and
    // propotion as fraction of cluster size
    
    int k = (int)components.size();
    
    if (k == 1) {
        // In case clusters are spatially contiguous, subcluster
        // fragmentation should not be computed
        frag.is_spatially_contiguous = true;
        return frag;
    }
    
    // max and min size of contiguous subcluster, mean size
    int min_size = 0, max_size = 0, mean_size = k / (double)GetSize();
    
    for (int i = 0; i < k; ++i) {
        int sz = components[i]->GetSize();
        if (i == 0 || sz < min_size) {
            min_size = sz;
        }
        if (i == 0 || sz > max_size) {
            max_size = sz;
        }
    }
    
    int n = (int)this->GetSize();
    double entropy = 0;
    
    // entropy measure applied to contiguous subclusters
    // measures are comparable for different numbers of subclusters
    for (int i = 0; i < k; ++i) {
        // number of observations in cluster, fraction of total in cluster
        int n_i = components[i]->GetSize();
        double p_i = n_i / (double) n;
        entropy -= p_i * log(p_i);
    }
    
    // max for k clusters = ln(k)
    double max_entropy = log((double)k);
    
    entropy = entropy / max_entropy;
    // higher value suggests more balanced cluster
    // smaller value suggests more inequality
    
    // Simpson index
    double simpson = 0;
    
    for (int i = 0; i < k; ++i) {
        int n_i = components[i]->GetSize();
        double p_i = n_i / (double) n;
        simpson += p_i * p_i;
    }
    
    double min_s = 1 / (double)k;
    
    // ratio of S/(1/k), smaller is more diverse (balanced)
    simpson = simpson / min_s;
    
    frag.n = k;
    frag.entropy = entropy;
    frag.simpson = simpson;
    frag.min_cluster_size = min_size;
    frag.max_cluster_size = max_size;
    frag.mean_cluster_size = mean_size;
    
    return frag;
}

Compactness SpatialValidationCluster::ComputeCompactness()
{
    // isoperimeter quotient (IPC)
    // only implement for spatially constrained result
    Compactness comp;
    
    // ratio of area of cluster to area of circle with equal perimeter
    std::vector<int> elements = GetCoreElements();
    
    if (components.size() != 1 || elements.empty()) {
        return comp;
    }
        
    double area = 0.0, perimeter = 0.0, ipc = 0.0;

    if (shape_type == Shapefile::POLYGON) {
        for (int i = 0; i < (int)elements.size(); ++i) {
            int idx = elements[i];
            OGRwkbGeometryType eType = wkbFlatten(geoms[i]->getGeometryType());
            if (eType == wkbPolygon || eType == wkbCurvePolygon) {
                OGRPolygon* p = (OGRPolygon *) geoms[idx];
                area += p->get_Area();
                
                OGRLinearRing* ring = p->getExteriorRing();
                perimeter += ring->get_Length();
                
            } else if (eType == wkbMultiPolygon) {
                OGRMultiPolygon* mpolygon = (OGRMultiPolygon *) geoms[idx];
                int n_geom = mpolygon->getNumGeometries();
                for (int j = 0; j < n_geom; j++ )
                {
                    OGRGeometry* ogrGeom = mpolygon->getGeometryRef(j);
                    OGRPolygon* p = static_cast<OGRPolygon*>(ogrGeom);
                    area += p->get_Area();
                    
                    OGRLinearRing* ring = p->getExteriorRing();
                    perimeter += ring->get_Length();
                }
            }
        }
    
    } else if (shape_type == Shapefile::POINT_TYP) {
        // For points: use area and perimeter of convex hull around points
        
        OGRGeometryCollection* points = (OGRGeometryCollection*)OGRGeometryFactory::createGeometry(wkbGeometryCollection);
        for (int i = 0; i < (int)elements.size(); ++i) {
            int idx = elements[i];
            OGRPoint* p = (OGRPoint *) geoms[idx];
            points->addGeometry(p);
        }
        OGRPolygon* hull = (OGRPolygon*)points->ConvexHull();
        area = hull->get_Area();
        
        OGRLinearRing* ring = hull->getExteriorRing();
        perimeter = ring->get_Length();
    }
    
    if (perimeter > 0) {
        ipc = 4 * M_PI * area / (perimeter * perimeter);
    }
    
    comp.isoperimeter_quotient = ipc;
    comp.area = area;
    comp.perimeter = perimeter;
    
    return comp;
}

Diameter SpatialValidationCluster::ComputeDiameter()
{
    // implement for spatially constrained results
    // diameter = longest shortest path between any pair in cluster
    
    Diameter diam;
    
    if (components.size() != 1) {
        return diam;
    }
    
    return core->ComputeDiameter();
}


SpatialValidation::SpatialValidation(int num_obs, const std::vector<std::vector<int> >& clusters, GeoDaWeight* weights, std::vector<OGRGeometry*>& geoms, Shapefile::ShapeType shape_type)
: num_obs(num_obs), clusters(clusters), weights(weights), valid(true), geoms(geoms),
shape_type(shape_type)
{
    num_clusters = (int)clusters.size();
    
    std::vector<int>::iterator it;
    for (int i=0; i < num_clusters; ++i) {
        std::vector<int> cluster = clusters[i];
        for (it = cluster.begin(); it != cluster.end(); ++it) {
            int j = *it;
            cluster_dict[j] = i;
        }
    }
    
    // check if clusters are valid
    if (cluster_dict.size() != num_obs) {
        valid = false;
    }
    
    // create SpatialValidationCluster for each cluster
    for (int i=0; i < num_clusters; ++i) {
        sk_clusters.push_back(new SpatialValidationCluster(i, clusters[i], weights, cluster_dict, geoms, shape_type));
    }
    
    ComputeFragmentation();
    ComputeCompactness();
    ComputeDiameter();
}

SpatialValidation::~SpatialValidation()
{
    for (int i = 0; i < (int)sk_clusters.size(); ++i) {
        delete sk_clusters[i];
    }
}

void SpatialValidation::ComputeFragmentation()
{
    // for the overall cluster result
    // provide list with cluster sizes for each cluster
    double entropy = 0;
    int k = (int) sk_clusters.size();
    int min_size = 0, max_size  = 0, mean_size = 0;
    
    for (int i = 0; i < k; ++i) {
        // number of observations in cluster, fraction of total in cluster
        int n_i = sk_clusters[i]->GetSize();
        double p_i = n_i / (double) num_obs;
        entropy -= p_i * log(p_i);
        
        int n_subclusters = sk_clusters[i]->GetComponentSize();
        
        if (i == 0 || n_subclusters < min_size) {
            min_size = n_subclusters;
        }
        if (i == 0 || n_subclusters > max_size) {
            max_size = n_subclusters;
        }
        mean_size += n_subclusters;
    }
    mean_size = k > 0 ? mean_size / (double) k : 0;
    
    // max for k clusters = ln(k)
    double max_entropy = log((double)k);
    
    // compute fraction entropy/ln(k) [so positive number since entropy
    // and max are both negative]
    
    entropy = entropy / max_entropy;
    // higher value suggests more balanced cluster
    // smaller value suggests more inequality
    
    // Simpson index
    double simpson = 0;
    
    for (int i = 0; i < k; ++i) {
        int n_i = sk_clusters[i]->GetSize();
        double p_i = n_i / (double) num_obs;
        simpson += p_i * p_i;
    }
    
    double min_s = 1 / (double)k;
    
    // ratio of S/(1/k), smaller is more diverse (balanced)
    simpson = simpson / min_s;
    
    fragmentation.n = k;
    fragmentation.entropy = entropy;
    fragmentation.simpson = simpson;
    fragmentation.min_cluster_size = min_size;
    fragmentation.max_cluster_size = max_size;
    fragmentation.mean_cluster_size = mean_size;
    
    // compute Fragmentation for each cluster
    for (int i = 0; i < k; ++i) {
        Fragmentation frag = sk_clusters[i]->ComputeFragmentation();
        fragmentations.push_back(frag);
    }
}

bool SpatialValidation::IsSpatiallyConstrained()
{
    int k = (int) sk_clusters.size();
    
    for (int i = 0; i < k; ++i) {
        if (sk_clusters[i]->GetComponentSize() != 1) {
            return false;
        }
    }
    
    return true;
}

void SpatialValidation::ComputeCompactness()
{
    // differentiate between polygon and point case
    // only implement for spatially constrained results
    
    if (IsSpatiallyConstrained()) {
        // compute for each cluster
        // as a summary report min, max, mean and range
        
        for (int i = 0; i < num_clusters; ++i) {
            Compactness comp = sk_clusters[i]->ComputeCompactness();
            compactnesses.push_back(comp);
        }
    }
}

void SpatialValidation::ComputeDiameter()
{
    // only implement for spatially constrained results
    if (IsSpatiallyConstrained()) {
        
        // Using spatial weights as a graph. Compute for each cluster
        for (int i = 0; i < num_clusters; ++i) {
            Diameter diam = sk_clusters[i]->ComputeDiameter();
            diameters.push_back(diam);
        }
    }
}
