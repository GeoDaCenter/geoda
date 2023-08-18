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

#include "../ShapeOperations/GalWeight.h"

#include <boost/thread/mutex.hpp>
#include <boost/unordered_map.hpp>
#include <boost/heap/priority_queue.hpp>
#include <boost/graph/adjacency_list.hpp>

namespace SpanningTreeClustering {
    
    class Node;
    class Edge;
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
        
        double ComputeSSD(std::vector<int>& visited_ids, int start, int end);
        void MeasureSplit(double ssd, std::vector<int>& visited_ids, int split_position, Measure& result);
        
    };
    
    /////////////////////////////////////////////////////////////////////////
    //
    // Node
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
                //cout << "AddNeighbor() > 2" << std::endl;
            }
        }
    };
    
    class Node
    {
    public:
        Node(int id);
        ~Node() {}
        
        
        int id; // mapping to record id
        Node* parent;
        int rank;
        
        //Cluster* container;
        //NeighborInfo nbr_info;
    };
    
    class DisjoinSet
    {
        boost::unordered_map<int, Node*> map;
    public:
        DisjoinSet();
        DisjoinSet(int id);
        ~DisjoinSet() {};
        
        Node* MakeSet(int id);
        void Union(Node* n1, Node* n2);
        Node* FindSet(Node* node);
    };
                   
    /////////////////////////////////////////////////////////////////////////
    //
    // Edge
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
    
    /////////////////////////////////////////////////////////////////////////
    //
    // Tree
    //
    /////////////////////////////////////////////////////////////////////////
    struct SplitSolution
    {
        int split_pos;
        std::vector<int> split_ids;
        double ssd;
        double ssd_reduce;
    };
    
    class Tree
    {
    public:
        Tree(std::vector<int> ordered_ids,
                   std::vector<Edge*> _edges,
                   AbstractClusterFactory* cluster);
        
        ~Tree();
        
        void Partition(int start, int end, std::vector<int>& ids,
                       std::vector<std::pair<int, int> >& od_array,
                       boost::unordered_map<int, std::vector<int> >& nbr_dict);
        void Split(int orig, int dest,
                   boost::unordered_map<int, std::vector<int> >& nbr_dict,
                   std::vector<int>& cand_ids);
        bool checkControl(std::vector<int>& cand_ids, std::vector<int>& ids, int flag);
        std::pair<Tree*, Tree*> GetSubTrees();
        
        double ssd_reduce;
        double ssd;
        
        std::vector<std::pair<int, int> > od_array;
        AbstractClusterFactory* cluster;
        std::pair<Tree*, Tree*> subtrees;
        int max_id;
        int split_pos;
        std::vector<int> split_ids;
        std::vector<Edge*> edges;
        std::vector<int> ordered_ids;
        SSDUtils* ssd_utils;
        
        double* controls;
        double control_thres;
        
        // threads
        boost::mutex mutex;
        void run_threads(std::vector<int>& ids,
                       std::vector<std::pair<int, int> >& od_array,
                       boost::unordered_map<int, std::vector<int> >& nbr_dict);
        std::vector<SplitSolution> split_cands;
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
    
    typedef boost::heap::priority_queue<Tree*, boost::heap::compare<CompareTree> > PriorityQueue;
    
    class AbstractClusterFactory
    {
    public:
        int rows;
        int cols;
        GalElement* w;
        double** dist_matrix;
        double** raw_data;
        const std::vector<bool>& undefs; // undef = any one item is undef in all variables
        double* controls;
        double control_thres;
        SSDUtils* ssd_utils;
        
        //Cluster* cluster;
        DisjoinSet djset;
        
        std::vector<Node*> nodes;
        std::vector<Edge*> edges;
        
        std::vector<int> ordered_ids;
        std::vector<Edge*> ordered_edges;
        
        std::vector<boost::unordered_map<int, double> > dist_dict;
        
        std::vector<std::vector<int> > cluster_ids;
        
        AbstractClusterFactory(int row, int col,
                       double** distances,
                       double** data,
                       const std::vector<bool>& undefs,
                       GalElement * w);
        virtual ~AbstractClusterFactory();
        
        virtual void Clustering()=0;
        
        virtual double UpdateClusterDist(int cur_id, int orig_id, int dest_id,
                                         bool is_orig_nbr, bool is_dest_nbr,
                                         std::vector<int>& clst_ids,
                                         std::vector<int>& clst_startpos,
                                         std::vector<int>& clst_nodenum) { return 0;}
        
        Edge* GetShortestEdge(std::vector<Edge*>& edges, int start, int end){ return NULL;}
        
        void init();
        void Partitioning(int k);
        std::vector<std::vector<int> >& GetRegions();
    };
    
    ////////////////////////////////////////////////////////////////////////////////
    //
    // 1 Skater
    //
    ////////////////////////////////////////////////////////////////////////////////
    typedef boost::adjacency_list <
    boost::vecS,
    boost::vecS,
    boost::undirectedS,
    boost::no_property,         //VertexProperties
    boost::property <boost::edge_weight_t, double>   //EdgeProperties
    > Graph;
    
    class Skater : public AbstractClusterFactory
    {
    public:
        Skater(int rows, int cols,
               double** _distances,
               double** data,
               const std::vector<bool>& undefs,
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
                            const std::vector<bool>& undefs,
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
                            const std::vector<bool>& undefs,
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
                            const std::vector<bool>& undefs,
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
                           const std::vector<bool>& undefs,
                           GalElement * w,
                           double* controls,
                           double control_thres,
                           bool init=true);
        
        virtual ~FullOrderALKRedCap();
        
        virtual void Clustering();
        
        virtual double UpdateClusterDist(int cur_id, int orig_id, int dest_id, bool is_orig_nbr, bool is_dest_nbr, std::vector<int>& clst_ids, std::vector<int>& clst_startpos, std::vector<int>& clst_nodenum);
        
        Edge* GetShortestEdge(std::vector<Edge*>& edges, int start, int end);
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
                           const std::vector<bool>& undefs,
                           GalElement * w,
                           double* controls,
                           double control_thres);
        virtual ~FullOrderSLKRedCap();
        
        virtual double UpdateClusterDist(int cur_id, int orig_id, int dest_id, bool is_orig_nbr, bool is_dest_nbr, std::vector<int>& clst_ids, std::vector<int>& clst_startpos, std::vector<int>& clst_nodenum);
        
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
                           const std::vector<bool>& undefs,
                           GalElement * w,
                           double* controls,
                           double control_thres);
        
        virtual ~FullOrderCLKRedCap();
        
        virtual double UpdateClusterDist(int cur_id, int orig_id, int dest_id, bool is_orig_nbr, bool is_dest_nbr, std::vector<int>& clst_ids, std::vector<int>& clst_startpos, std::vector<int>& clst_nodenum);
        
    };

    ////////////////////////////////////////////////////////////////////////////////
    //
    // 6 Ward
    //
    ////////////////////////////////////////////////////////////////////////////////
    class FullOrderWardRedCap : public FullOrderALKRedCap
    {
    public:
        FullOrderWardRedCap(int rows, int cols,
                           double** _distances,
                           double** data,
                           const std::vector<bool>& undefs,
                           GalElement * w,
                           double* controls,
                           double control_thres);
        
        virtual ~FullOrderWardRedCap();
        
        virtual void Clustering();
        
        virtual double UpdateClusterDist(int cur_id, int orig_id, int dest_id, double min_dist, bool is_orig_nbr, bool is_dest_nbr, std::vector<int>& clst_ids, std::vector<int>& clst_startpos, std::vector<int>& clst_nodenum, std::vector<int>& ids);
    };
}

#endif
