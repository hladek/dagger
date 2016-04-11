/*
 * BigramModel.h
 *
 *  Created on: 15.5.2013
 *      Author: voicegroup
 */

#ifndef BIGRAMMODEL_H_
#define BIGRAMMODEL_H_

#include "Ngrams.h"
#include "Vocab3.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <sstream>


/**
 * Trigram Language Model
 */
class BigramModel: public LogModelI {

	//Unigrams unigrams;
	//Ngrams2<IntKey<2>,1,Unigrams> bigrams;
	void init();
	void shutdown();
	

public:
    Vocab3& vocab;
	bool externalVocabulary;
	std::vector<NgramCounter*> ngrams;

	BigramModel(Vocab3& vocab);
	virtual ~BigramModel();

	//void calculate_prob();
	
	void reset();

	
	void load_arpa(const char* filename);
	void save_arpa(const char* filename);

	virtual double get_prob(int* key) const;
    
    // Adds sentence to counts
    void learn_item(int* item,size_t sz) {
        for (size_t j = 0; j < order; j++){
		    for(long i = 0; i < (long)sz - (long)j; i++){
                ngrams[j]->increase_count(item + i,1.0);
            }
		}
	}

	
	virtual void serialize(ostream& os);
	virtual void deserialize(istream& os);

};

class Perplexity {
	LogModelI& bm;
	double sumprob;
	int zeroprobs;
	int numwords;
public:
	Perplexity(LogModelI& bm) :
		bm(bm) {
		sumprob = 0;
		zeroprobs = 0;
		numwords = 0;
	}
	virtual ~Perplexity() {
	}
	;
	virtual void process_item(const vector<int>& item) {
		double prob = 0;

		vector<int> h(bm.order - 1, -1);
		for (size_t i = 0; i < item.size(); i++) {
			h.push_back(item[i]);
			prob = bm.get_prob((int*) h.data() + i);
			if (prob <= -99) {
				zeroprobs += 1;
			} else {
				sumprob += prob;
				numwords += 1;

			}
			//cout << prob << endl;
		}

	}
	virtual void process() {
		cout << sumprob << " " << numwords << endl;
		double perplexity = pow(10.0, -sumprob / (double) numwords);
		printf("ppl: %f zeroprobs: %d (%f %%)\n", perplexity, zeroprobs,
				(double) zeroprobs / (double) (numwords + zeroprobs) * 100.0);
		sumprob = 0;
		zeroprobs = 0;
		numwords = 0;
	}
};

class ADLearner {
public:
	BigramModel& bm;
	ADLearner(BigramModel& bm) :
		bm(bm) {

	}
	virtual ~ADLearner() {
	}
	
    
	virtual void process() {
        bm.ngrams[0]->calculate_prob();
		for (size_t j = 1; j < bm.order; j++) {
        	double c = 0.0;
        	double prob = 0.0;
          	double n1 = 0.001;
         	double n2 = 0.001;
        	double sumCount = 0;
        	double sumProb = 0;
        	int zeroCount = 0;
            int count_index = bm.ngrams[j]->count_index;
			// Calculate 1 and 2 frequencies
			//assert( this->size() > 0);
	        BtreeIterator it = bm.ngrams[j]->iterator();
        	while(it.next()){
		        c =  it.double_value()[count_index];
		        if (c == 1.0){
			        n1 += 1;
		        }
		        else if (c == 2.0){
		    	    n2 += 1;
		        }
		        else if (c == 0.0){
			        zeroCount +=1;
		        }
		        sumCount += c;
	        }
        	assert(sumCount > 0.0);
			// Calculate b

	        double b = n1 / (n1 + 4* n2);
	        //std::cout << "n1 " << n1 << " n2 " << n2 <<  " ZeroCount " << zeroCount << " b " << b << " Items " << size() <<std::endl;
	        it = bm.ngrams[j]->iterator();
            assert(b > 0 && b < 1.0);
	        while(it.next()){
		        c =  it.double_value()[count_index];
		        assert(c > 0);
                double histCount = bm.ngrams[j-1]->get_count(it.key());
		        assert(histCount > 0);
		        assert(c <= histCount);
		        prob = (c-b)  / histCount;
                if (prob >= 1.0) {
		            cout << prob << " " << b << " " << c << " " << histCount  <<  endl;
                    abort();
                }
		        sumProb += prob;
		        prob = log10(prob);
		        it.double_value()[0] = prob;
	        }
    //cerr << "Problems " << problems << endl;
        }
		bm.ngrams.back()->renormalize();
	}

};



class KNLearner : public ADLearner {
public:
	KNLearner(BigramModel& bm) :
		ADLearner(bm) {

	}
	virtual ~KNLearner() {
	};
	virtual void process() {
        bm.ngrams[0]->calculate_prob();
		for (size_t j = 1; j < bm.order; j++) {
        	double c = 0.0;
        	double prob = 0.0;
          	double n1 = 0.001;
         	double n2 = 0.001;
        	double sumCount = 0;
        	double sumProb = 0;
        	int zeroCount = 0;
            NgramCounter ng(j,2);
            int count_index = bm.ngrams[j]->count_index;
			// Calculate 1 and 2 frequencies
			//assert( this->size() > 0);
	        BtreeIterator it = bm.ngrams[j]->iterator();
        	while(it.next()){
		        c =  it.double_value()[count_index];
                pair<bool,BtreeIterator> res = ng.insert(it.key());
                res.second.double_value()[0] += 1;
                res.second.double_value()[1] += c;
		        if (c == 1.0){
			        n1 += 1;
		        }
		        else if (c == 2.0){
		    	    n2 += 1;
		        }
		        else if (c == 0.0){
			        zeroCount +=1;
		        }
		        sumCount += c;
	        }
        	assert(sumCount > 0.0);
			// Calculate b

	        double b = n1 / (n1 + 4* n2);
	        //std::cout << "n1 " << n1 << " n2 " << n2 <<  " ZeroCount " << zeroCount << " b " << b << " Items " << size() <<std::endl;
	        it = bm.ngrams[j]->iterator();
            assert(b > 0 && b < 1.0);
	        while(it.next()){
		        c =  it.double_value()[count_index];
		        assert(c > 0);
		        BtreeIterator hit = ng.find(it.key());
		        assert(hit.has_value());
                double n1plus = hit.double_value()[0];
		        double histCount = hit.double_value()[1] ;
                if (histCount == 0){
                    cout << j << " " << c << endl;
                    cout << it.key()[0]  << " " << it.key()[1] << endl;
                    abort();
                }
		        assert(histCount > 0);
		        assert(c <= histCount);
                hit = bm.ngrams[j-1]->find(it.key() + 1);
                double backprob = pow(10.0,hit.double_value()[0]);
		        prob = ((c-b) + b * n1plus * backprob) / histCount;
                if (prob >= 1.0) {
		            cout << prob << " " << b << " " << c << " " << histCount  << " " << n1plus << endl;
                    abort();
                }
		        sumProb += prob;
		        prob = log10(prob);
		        it.double_value()[0] = prob;
	        }
    //cerr << "Problems " << problems << endl;
        }
		bm.ngrams.back()->renormalize();
	}

};

class MDIAdapt {
	BigramModel& bm;
	NgramCounter uc;
	float gama;
	int wc;

public:
	MDIAdapt(BigramModel& bm,float gama) :
			bm(bm),uc(1) {
		this->gama = gama;
		assert(gama > 0 && gama <= 1.0);
		wc = 0;
		}
		virtual ~MDIAdapt(){};
            virtual void process() {
		// Podla M. Federico
		// http://pdf.aminer.org/003/048/037/efficient_language_model_adaptation_through_mdi_estimation.pdf
		NgramCounter alpha(1);
		float sumProb = 0;
		float discount = 0;
		BtreeIterator it = bm.ngrams[0]->iterator();
		double ucnorm = log10((double)wc);
		while(it.next()){
			float c = 0;
			BtreeIterator bit = uc.find(it.key());
			c = gama * (-ucnorm - it.double_value()[0]);
			if (bit.has_value()){
				c = gama * (log10((double)bit.double_value()[0] ) - ucnorm   - it.double_value()[0]);
			}
			alpha.increase_count(it.key(),c);
			discount += pow(10.0,it.double_value()[0]);
			it.double_value()[0] += c;
			sumProb += pow(10.0,it.double_value()[0]);
		}
		cout << sumProb << " " << discount << endl;
		discount = log10((double)discount);
		sumProb = log10((double)sumProb);
		it = bm.ngrams[0]->iterator();
		while(it.next()){
			it.double_value()[0] +=  (discount -sumProb);
		}
		// Uprava pravdepodobnosti ngramov
		for (size_t j = 1; j < 2; j++) {
			NgramCounter nor(j,2);
			it = bm.ngrams[j]->iterator();
			while (it.next()){
				double v = it.double_value()[0];
				double a = alpha.get_count(it.key() + j);
				pair<bool,BtreeIterator> it2 = nor.insert(it.key());
				if (it2.first == true){
					it2.second.double_value()[0] = pow(10.0,v);
					it2.second.double_value()[1] = pow(10.0,v + a);
				}
				else{
					it2.second.double_value()[0] += pow(10.0,v);
					it2.second.double_value()[1] += pow(10.0,v + a);
				}
				if (it2.second.double_value()[0] <= 0.0 || it2.second.double_value()[1] <= 0.0){
					cerr << "Norm error " << v << " " << a << endl;
					abort();
				}
				it.double_value()[0] += a;
				if (!isfinite(it.double_value()[0])){
					abort();
				}
			}
			it = bm.ngrams[j]->iterator();
			while (it.next()){
				BtreeIterator it2 = nor.find(it.key());
				assert(it2.has_value());
				if(!it2.has_value() || it2.double_value()[0] <= 0.0 || it2.double_value()[1] <= 0.0 ){
					cout << "MDI nOrmalization error; " << it.double_value()[0] << " " << it2.has_value() << " " << it2.double_value()[0] << " " <<  it2.double_value()[1] << endl;
					//abort();
				}
				it.double_value()[0] +=  (log10(it2.double_value()[0]) - log10(it2.double_value()[1]));
			}
		}
		// Pocitanie backoff vah pre cely model
		bm.ngrams.back()->renormalize();

	}

};



#endif /* BIGRAMMODEL_H_ */
