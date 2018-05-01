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

#ifndef __GEODA_CENTER_HDBSCAN_H__
#define __GEODA_CENTER_HDBSCAN_H__

#include "../kNN/ANN.h"

#include "redcap.h"

using namespace SpanningTreeClustering;

namespace GeoDaClustering {
    class UnionFind
    {
    public:
        UnionFind* parent;
        int rank;
        int item;
        
    public:
        UnionFind(int _item) {
            parent = this;
            rank = 0;
            item = _item;
        }
        ~UnionFind() { }
        
        UnionFind* find() {
            if (parent != this) {
                parent = parent->find();
            }
            return parent;
        }
        
        int getItem() {
            return item;
        }
        
        void Union(UnionFind* y) {
            UnionFind* xRoot = find();
            UnionFind* yRoot = y->find();
            
            if (xRoot == yRoot) return;
            
            if (xRoot->rank < yRoot->rank)
                xRoot->parent = yRoot;
            else if (xRoot->rank > yRoot->rank)
                yRoot->parent = xRoot;
            else
            {
                yRoot->parent = xRoot;
                xRoot->rank = xRoot->rank + 1;
            }
        }
    };
    /////////////////////////////////////////////////////////////////////////
    //
    // HDBSCAN
    //
    /////////////////////////////////////////////////////////////////////////
    class HDBScan
    {
        int rows;
        int cols;
        
        DisjoinSet djset;
        vector<SpanningTreeClustering::Edge*> mst_edges;
        
        vector<Node*> all_nodes;
        vector<double> cores;
        vector<int> designations;
        vector<boost::unordered_map<int, double> > nbr_dict;
    public:
        HDBScan(int rows, int cols,
                double** _distances,
                double** data,
                const vector<bool>& undefs,
                GalElement * w,
                double* controls,
                double control_thres);
        virtual ~HDBScan();
        
    };
    
}
#endif
