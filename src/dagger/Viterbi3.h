#include "Ngrams.h"
#ifndef VIT3_H_
#define VIT3_H_

class ViterbiItem3 {
    public:
int curr;
double value;
int back;
};



class Viterbi3 {
    LogModelI& trans;
    Viterbi3* last;
    size_t count;
    int best_state;
    double best_value;
    static const int order = 3;

    public:
        vector<ViterbiItem3> states;
        Viterbi3(LogModelI& trans,Viterbi3* last) :
            trans(trans),
            last(last){
            if(last == NULL)
                count = 1;
            else
                count = last->count + 1;
            best_state = -1;
            best_value = -9999999;
            
        }
        virtual ~Viterbi3(){
            if(last != NULL){
              delete last;  
            }
        }
    
        void add_state(int next_state, double obsprob){
            ViterbiItem3 v;
            v.curr = next_state;
            v.value = -9999999999;
            v.back = -1;
            int key[order];
            fill(key,key+order,-1);
            if (last == NULL){
                key[order-1] = next_state;
                v.value = obsprob +  trans.get_prob(key);
            }
            else{
                if (order > 2 && last->last != NULL){
                    key[order-3] = last->last->best_state;
                    if (order > 3 && last->last->last != NULL){
                        key[order-4] = last->last->last->best_state;
                    }
                }
                for (size_t i = 0; i < last->states.size() ; i++){
                    ViterbiItem3& lv = last->states[i];
                    key[order-2] = lv.curr;
                    key[order-1] = next_state;
                    double p = trans.get_prob(key) + lv.value + obsprob;
                    if (p >= v.value){
                       v.value = p;
                       v.back = i;
                    }
                }
            }
            states.push_back(v);
            if (v.value > best_value){
                best_state = next_state;
                best_value = v.value;
            }
        }
    
    vector<int> backtrack(){
        vector<int> res(count);
        double v = -100000000;
        int bi = -1;
        int bs = -1;
        for (size_t i = 0; i < states.size(); i++ ){
            if (states[i].value > v){
                v = states[i].value;
                bi = states[i].back;
                bs = states[i].curr;
            }
        }
        res[count - 1]= bs;
        Viterbi3* lv = last;
        while(lv != NULL){
            res[lv->count-1] = lv->states[bi].curr;
            bi = lv->states[bi].back;
            lv = lv->last;
        }
        return res;
    }
};
#endif
