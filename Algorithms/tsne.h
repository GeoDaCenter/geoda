/*
 *  tsne.h
 *  Header file for t-SNE.
 *
 *  Created by Laurens van der Maaten.
 *  Copyright 2012, Delft University of Technology. All rights reserved.
 *
 *  Multicore version by Dmitry Ulyanov, 2016. dmitry.ulyanov.msu@gmail.com
 */


#ifndef TSNE_H
#define TSNE_H

#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>


#include "vptree.h"

static inline double sign(double x) { return (x == .0 ? .0 : (x < .0 ? -1.0 : 1.0)); }

class TSNE
{
public:
    TSNE(double* X, int N, int D, double* Y,
         int no_dims = 2, double perplexity = 30, double theta = .5,
         int num_threads = 1, int max_iter = 1000, 
         int n_iter_early_exag = 250,
         unsigned int random_state = 0, bool init_from_Y = false, int verbose = 0,
         double early_exaggeration = 12, double learning_rate = 200,
         double *final_error = NULL);

    void stop();
    void set_paused(bool new_value);
    void set_speed(int speed);
    void run(boost::lockfree::queue<int>& tsne_queue,
             std::vector<std::string>& tsne_log,
             std::vector<std::vector<double> >& results);

    void symmetrizeMatrix(int** row_P, int** col_P, double** val_P, int N);
    
private:
    double computeGradient(int* inp_row_P, int* inp_col_P, double* inp_val_P, double* Y, int N, int D, double* dC, double theta, bool eval_error);
    double evaluateError(int* row_P, int* col_P, double* val_P, double* Y, int N, int no_dims, double theta);
    void zeroMean(double* X, int N, int D);
    void computeGaussianPerplexity(double* X, int N, int D, int** _row_P, int** _col_P, double** _val_P, double perplexity, int K, int verbose);
    double randn();

    double* X;
    int N;
    int D;
    double* Y;
    int no_dims;
    double perplexity;
    double theta;
    int num_threads;
    int max_iter;
    int n_iter_early_exag;
    unsigned int random_state;
    bool skip_random_init;
    int verbose;
    double early_exaggeration;
    double learning_rate;
    double *final_error;
    int *act_iter;
    std::string* report;

    bool is_stop;
    int m_speed;
    bool m_pause; // initialise to false in constructor!
    boost::mutex m_pause_mutex;
    boost::condition_variable m_pause_changed;
};

#endif

