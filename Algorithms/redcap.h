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
 * Created: 5/30/2017 lixun910@gmail.com
 */

#ifndef __GEODA_CENTER_REDCAP_H__
#define __GEODA_CENTER_REDCAP_H__

#include <vector>
#include <set>
#include <float.h>
#include <boost/thread/mutex.hpp>
#include <boost/unordered_map.hpp>
#include <boost/heap/priority_queue.hpp>
#include <boost/graph/adjacency_list.hpp>

#include "../ShapeOperations/GalWeight.h"


using namespace std;
using namespace boost;

namespace SpanningTreeClustering {
    
    class Node;
    class Edge;
    class Cluster;
    class Tree;
    class AbstractClusterFactory;
    
    /////////////////////////////////////////////////////////////////////////
    //
    // SSDUtils
    //
    /////////////////////////////////////////////////////////////////////////
    struct Measure
    {
        double ssd;
        double ssd_part1;
        double ssd_part2;
        double measure_reduction;
    };
    
    class SSDUtils
    {
        double** raw_data;
        int row;
        int col;
        
    public:
        SSDUtils(double** data, int _row, int _col) {
            raw_data = data;
            row = _row;
            col = _col;
        }
        ~SSDUtils() {}
        
        double ComputeSSD(vector<int>& visited_ids, int start, int end);
        void MeasureSplit(double ssd, vector<int>& visited_ids, int split_position, Measure& result);
        
    };
    
    /////////////////////////////////////////////////////////////////////////
    //
    // RedCapNode
    //
    /////////////////////////////////////////////////////////////////////////
    
    struct NeighborInfo
    {
        Node* p;
        Node* n1;
        Node* n2;
        Edge* e1;
        Edge* e2;
        
        void SetDefault(Node* parent) {
            p = parent;
            n1 = NULL;
            n2 = NULL;
            e1 = NULL;
            e2 = NULL;
        }
        
        void AddNeighbor(Node* nbr, Edge* e) {
            if (n1 == NULL) {
                n1 = nbr;
                e1 = e;
            } else if (n2 == NULL) {
                n2 = nbr;
                e2 = e;
            } else {
                cout << "AddNeighbor() > 2" << endl;
            }
        }
    };
    
    class Node
    {
    public:
        Node(int id);
        ~Node() {}
        
        void SetCluster(Cluster* cluster);
        void AddNeighbor(Node* nbr, Edge* e);
        
        int id; // mapping to record id
        Cluster* container;
        NeighborInfo nbr_info;
    };
    
    /////////////////////////////////////////////////////////////////////////
    //
    // RedCapEdge
    //
    /////////////////////////////////////////////////////////////////////////
    class Edge
    {
    public:
        Edge(Node* a, Node* b, double length);
        ~Edge() {}
        
        Node* orig;
        Node* dest;
        double length; // legnth of the edge |a.val - b.val|
    };
    
    ////////////////////////////////////////////////////////////////////////////////
    //
    // RedCapCluster
    //
    ////////////////////////////////////////////////////////////////////////////////
    class Cluster {
    public:
        static Cluster* GetRoot(Node* node);
        static void GetOrderedNodes(Cluster* root, vector<Node*>& ordered_nodes);
        
        Cluster(Node* node);
        Cluster(Cluster* c1, Cluster* c2, Edge* e, double** dist_matrix);
        ~Cluster() {
            if (child1) {
                delete child1;
            }
            if (child2) {
                delete child2;
            }
        }
        
        
        double** dist_matrix;
        
        Node* p1;
        Node* p2;
        Cluster* parent;
        Cluster* child1;
        Cluster* child2;
    };
    
    
    
    /////////////////////////////////////////////////////////////////////////
    //
    // RedCapTree
    //
    /////////////////////////////////////////////////////////////////////////
    
    
    class Tree
    {
    public:
        Tree(vector<int> ordered_ids,
                   vector<Edge*> _edges,
                   AbstractClusterFactory* redcap);
        
        ~Tree();
        
        void Partition(vector<int>& ids,
                       vector<pair<int, int> >& od_array,
                       unordered_map<int, vector<int> >& nbr_dict);
        void Split(int orig, int dest,
                   unordered_map<int, vector<int> >& nbr_dict,
                   vector<int>& cand_ids);
        bool checkControl(vector<int>& cand_ids);
        pair<Tree*, Tree*> GetSubTrees();
        
        double ssd_reduce;
        double ssd;
        
        vector<pair<int, int> > od_array;
        AbstractClusterFactory* redcap;
        pair<Tree*, Tree*> subtrees;
        int max_id;
        int split_pos;
        vector<int> split_ids;
        vector<Edge*> edges;
        vector<int> ordered_ids;
        SSDUtils* ssd_utils;
        
        
        double* controls;
        double control_thres;
    };
    
    ////////////////////////////////////////////////////////////////////////////////
    //
    // AbstractRedcap
    //
    ////////////////////////////////////////////////////////////////////////////////
    struct CompareTree
    {
    public:
        bool operator() (const Tree* lhs, const Tree* rhs) const
        {
            return lhs->ssd_reduce < rhs->ssd_reduce;
        }
    };
    
    typedef heap::priority_queue<Tree*, heap::compare<CompareTree> > PriorityQueue;
    
    class AbstractClusterFactory
    {
    public:
        int rows;
        int cols;
        GalElement* w;
        double** dist_matrix;
        double** raw_data;
        const vector<bool>& undefs; // undef = any one item is undef in all variables
        double* controls;
        double control_thres;
        SSDUtils* ssd_utils;
        
        Cluster* cluster;
        
        vector<Node*> nodes;
        vector<Edge*> edges;
        
        vector<int> ordered_ids;
        vector<Edge*> ordered_edges;
        
        vector<unordered_map<int, double> > dist_dict;
        
        vector<vector<int> > cluster_ids;
        
        AbstractClusterFactory(int row, int col,
                       double** distances,
                       double** data,
                       const vector<bool>& undefs,
                       GalElement * w);
        virtual ~AbstractClusterFactory();
        
        virtual void Clustering()=0;
        
        virtual double UpdateClusterDist(int cur_id, int orig_id, int dest_id, bool is_orig_nbr, bool is_dest_nbr, vector<int>& clst_ids, vector<int>& clst_startpos, vector<int>& clst_nodenum) { return 0;}
        
        Edge* GetShortestEdge(vector<Edge*>& edges, int start, int end){}
        
        void init();
        void Partitioning(int k);
        vector<vector<int> >& GetRegions();
    };
    
    ////////////////////////////////////////////////////////////////////////////////
    //
    // 1 Skater
    //
    ////////////////////////////////////////////////////////////////////////////////
    typedef adjacency_list <
    vecS,
    vecS,
    undirectedS,
    boost::no_property,         //VertexProperties
    property < edge_weight_t, double>   //EdgeProperties
    > Graph;
    
    class Skater : public AbstractClusterFactory
    {
    public:
        Skater(int rows, int cols,
               double** _distances,
               double** data,
               const vector<bool>& undefs,
               GalElement * w,
               double* controls,
               double control_thres);
        virtual ~Skater();
        virtual void Clustering();
    };
    
    ////////////////////////////////////////////////////////////////////////////////
    //
    // 1 FirstOrderSLKRedCap
    //
    ////////////////////////////////////////////////////////////////////////////////
    class FirstOrderSLKRedCap : public AbstractClusterFactory
    {
    public:
        FirstOrderSLKRedCap(int rows, int cols,
                            double** _distances,
                            double** data,
                            const vector<bool>& undefs,
                            GalElement * w,
                            double* controls,
                            double control_thres);
        virtual ~FirstOrderSLKRedCap();
        
        virtual void Clustering();
    };
    
    ////////////////////////////////////////////////////////////////////////////////
    //
    // 2 FirstOrderALKRedCap
    //
    ////////////////////////////////////////////////////////////////////////////////
    class FirstOrderALKRedCap : public AbstractClusterFactory
    {
    public:
        FirstOrderALKRedCap(int rows, int cols,
                            double** _distances,
                            double** data,
                            const vector<bool>& undefs,
                            GalElement * w,
                            double* controls,
                            double control_thres);
        
        virtual ~FirstOrderALKRedCap();
        
        virtual void Clustering();
        
    };
    
    ////////////////////////////////////////////////////////////////////////////////
    //
    // 3 FirstOrderCLKRedCap
    //
    ////////////////////////////////////////////////////////////////////////////////
    class FirstOrderCLKRedCap : public AbstractClusterFactory
    {
    public:
        FirstOrderCLKRedCap(int rows, int cols,
                            double** _distances,
                            double** data,
                            const vector<bool>& undefs,
                            GalElement * w,
                            double* controls,
                            double control_thres);
        
        virtual ~FirstOrderCLKRedCap();
        
        virtual void Clustering();
        
    };
    
    ////////////////////////////////////////////////////////////////////////////////
    //
    // 5 FullOrderALKRedCap
    //
    ////////////////////////////////////////////////////////////////////////////////
    class FullOrderALKRedCap : public AbstractClusterFactory
    {
    public:
        FullOrderALKRedCap(int rows, int cols,
                           double** _distances,
                           double** data,
                           const vector<bool>& undefs,
                           GalElement * w,
                           double* controls,
                           double control_thres,
                           bool init=true);
        
        virtual ~FullOrderALKRedCap();
        
        virtual void Clustering();
        
        virtual double UpdateClusterDist(int cur_id, int orig_id, int dest_id, bool is_orig_nbr, bool is_dest_nbr, vector<int>& clst_ids, vector<int>& clst_startpos, vector<int>& clst_nodenum);
        
        Edge* GetShortestEdge(vector<Edge*>& edges, int start, int end);
    };
    
    ////////////////////////////////////////////////////////////////////////////////
    //
    // 4 FullOrderSLKRedCap
    //
    ////////////////////////////////////////////////////////////////////////////////
    class FullOrderSLKRedCap : public FullOrderALKRedCap
    {
    public:
        FullOrderSLKRedCap(int rows, int cols,
                           double** _distances,
                           double** data,
                           const vector<bool>& undefs,
                           GalElement * w,
                           double* controls,
                           double control_thres);
        virtual ~FullOrderSLKRedCap();
        
        virtual double UpdateClusterDist(int cur_id, int orig_id, int dest_id, bool is_orig_nbr, bool is_dest_nbr, vector<int>& clst_ids, vector<int>& clst_startpos, vector<int>& clst_nodenum);
        
    };
    
    
    ////////////////////////////////////////////////////////////////////////////////
    //
    // 6 FullOrderCLKRedCap
    //
    ////////////////////////////////////////////////////////////////////////////////
    class FullOrderCLKRedCap : public FullOrderALKRedCap
    {
    public:
        FullOrderCLKRedCap(int rows, int cols,
                           double** _distances,
                           double** data,
                           const vector<bool>& undefs,
                           GalElement * w,
                           double* controls,
                           double control_thres);
        
        virtual ~FullOrderCLKRedCap();
        
        virtual double UpdateClusterDist(int cur_id, int orig_id, int dest_id, bool is_orig_nbr, bool is_dest_nbr, vector<int>& clst_ids, vector<int>& clst_startpos, vector<int>& clst_nodenum);
        
    };

}

#endif
