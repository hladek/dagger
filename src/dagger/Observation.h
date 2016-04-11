#ifndef _OBSERVATION__ 
#define _OBSERVATION__ 
#include "Ngrams.h"
/**
 * Probability model, where counts of items are stored in the memeory
 * and probability is calculated on demmand
 */

class Observation : public NgramCounter {
public:

    size_t backoff_offset;
    size_t hi;
	NgramCounter backoff;	
    
    Observation(size_t order,size_t backoff_offset, size_t history_offset):
		NgramCounter(order),
	    backoff(1)
    {
            assert(order > 1);
            assert(backoff_offset == 0 || backoff_offset == order -1);
            this->backoff_offset = backoff_offset;
            hi = history_offset; 

		};

	virtual ~Observation(){};
    virtual void reset(){};	
    virtual void clear(){};
    virtual void calculate_prob() {
        BtreeIterator it = iterator();
        double sumAll = 0;
        NgramCounter histcounts(order-1);
        while (it.next()){
            double v = it.double_value()[0];
            histcounts.increase_count(it.key() + hi,v);
            backoff.increase_count(it.key() + backoff_offset,v);
            sumAll += v;
        }

        it = iterator(); 
        // Deleted interpolation - vychadza horsie
        // Maly absolute discounting - vycxhadza lepsie
        backoff.calculate_prob();
        while (it.next()){
            BtreeIterator hit = histcounts.find(it.key() + hi);
            double histcount = hit.double_value()[0];
            double c = it.double_value()[0] -0.01;
            it.double_value()[0] = log10(c/histcount);
        }
    };
    virtual double get_prob(int *key) const{
        BtreeIterator it = find(key);
        if (it.has_value()){
            return it.double_value()[0];
        }
        else{
            BtreeIterator it2 = backoff.find(key+backoff_offset);
            if (it2.has_value()){
                return it2.double_value()[0];
            } 
        }
        return -99;
	}
    double get_prob(int c1,int c2) const {
        int key[2] = {c1,c2};
        return get_prob(key);
    }

	void serialize(ostream& os) {
        os.write((char*)&backoff_offset,sizeof(size_t));
        NgramCounter::serialize(os);
		backoff.serialize(os);
	};
	void deserialize(istream& os) {
        os.read((char*)&backoff_offset,sizeof(size_t));
        NgramCounter::deserialize(os);
		backoff.deserialize(os);
	}
};

class Trigrams : public LogModelI {
    public:
        vector<NgramCounter*> ngrams;
        size_t sumcount;
        Trigrams() : LogModelI(3){
            ngrams.push_back(new NgramCounter(1));
            ngrams.push_back(new NgramCounter(2));
            ngrams.push_back(new NgramCounter(3));
            sumcount = 0;
        }
        virtual ~Trigrams(){
            for (size_t i = 0; i < ngrams.size() ; i++){
                delete ngrams[i];
            }
        }
        virtual void process_item(int* item,size_t sz) {
            sumcount += sz;
            for (size_t j = 0; j < ngrams.size(); j++) {
                for (size_t i = 0; (i + j) < sz; i++) {
                    //cout << i << " " << item.size() -j << endl;
                    ngrams[j]->increase_count(item + i, 1);
                }
            }
	    }
        virtual void calculate_prob(){

            BtreeIterator it = ngrams.back()->iterator();
            vector<double> weights(3);
            while (it.next()){
                double num1 = ngrams[0]->get_count(it.key() + 2) -1;
                double v1 = num1 / (sumcount-1);
                double maxv = v1;
                int maxo = 1;
                double num2 = ngrams[1]->get_count(it.key() + 1) -1;
                double denum2 = ngrams[0]->get_count(it.key() + 1) -1;
                double v2 = 2;
                if (denum2 > 0 ){
                    v2 = num2 / denum2;
                }
                if (v2 >= maxv){
                    maxo = 2;
                    maxv = v2;
                }
                double v3 = 3;
                double num3 = it.double_value()[0] -1;
                double denum3 = ngrams[1]->get_count(it.key()) -1;
                if (denum3 > 0){
                    v3 = num3/denum3;
                }
                if (v3 > maxo){
                    maxo = 3;
                }
                weights[maxo-1] += num3 + 1;
           }
            it = ngrams[2]->iterator();
            while (it.next()){
                double prob = it.double_value()[0];
                double denum = ngrams[1]->get_count(it.key()) ;
                it.double_value()[0] =prob/denum; 
            }
            it = ngrams[1]->iterator();
            while (it.next()){
                double prob = it.double_value()[0];
                double denum = ngrams[0]->get_count(it.key()) ;
                it.double_value()[0] =prob/denum; 
            }
            it = ngrams[0]->iterator();
            while (it.next()){
                double prob = it.double_value()[0];
                it.double_value()[0] =log10(prob/sumcount * weights[0] / sumcount); 
            }
            it = ngrams[1]->iterator();
            while (it.next()){
                double prob = it.double_value()[0];
                double backoff = ngrams[0]->get_count(it.key() + 1);
                prob = log10(prob * weights[1] /sumcount + pow(10.0,backoff));
                it.double_value()[0] = prob;
            }
            it = ngrams[2]->iterator();
            while (it.next()){
                double prob = it.double_value()[0];
                double backoff = ngrams[1]->get_count(it.key() + 1);
                prob = log10(prob * weights[2] /sumcount + pow(10.0,backoff));
                it.double_value()[0] = prob;
            }
cout << weights[0] << " " << weights[1] << " " << weights[2] << endl;
        }
        virtual double get_prob(int* key) const{
            BtreeIterator it = ngrams[2]->find(key);
            if (it.has_value()){
                return it.double_value()[0];
            }
            else{
                it = ngrams[1]->find(key + 1);
                if (it.has_value()){
                    return it.double_value()[0];
                }
                else{
                    it = ngrams[0]->find(key + 2);
                    if (it.has_value()){
                        return it.double_value()[0];
                    }
                }

            }
            return -99;
        }
	virtual double increase_count(int* key,double weight) {
        return 0;
    }

	virtual void serialize(ostream& os){
    }
	virtual void deserialize(istream& os){
    }


};

#endif
