/*
 * Unigrams2.h
 *
 *  Created on: 14.11.2013
 *      Author: voicegroup
 */

#ifndef UNIGRAMS2_H_
#define UNIGRAMS2_H_

#include "btree.h"
#include "Vocab3.h"

inline void add_log(double &a,double b){
	//assert(a <= 0 && b <= 0);
	assert(isfinite(b));
	double c = a;
	//if (b != ZEROLOG){
	if (c > b){
		a += log10(1.0 + pow(10.0,b-a));
	}
	else if (c < b){
		a = b + log10(1.0 + pow(10.0,c-b));
	}
	else{
		a += log10(2.0);
	}
	assert(isfinite(a));
	assert(a >= c);
	assert(a >= b);
}
/**
 * Interface for a probabilistic model returning
 * log probability
 */
class LogModelI {
public:

	/**
	 * Order of the model
	 * dimension of the key array
	 */
    size_t order;
	/**
	 * Calculate log probability of given item
	 * @arg Inspected array of items of size order - can be
	 * [item] - pointer to item
	 * [context,state] - pointer to array
	 * [history,word]
	 * @return Logarithm with base 10 of probability, zeroLogProb if zero, 0 if 1
	 */
	virtual double get_prob(int* key) const  = 0;

    virtual ~LogModelI(){};
	/**
	 * Sets order of a model
	 */
	LogModelI(int order):order(order){
		assert(order > 0);
	};
	/**
	 * Copy constructor
	 *
	 * Asserts that copied model has the same order
	 */
	LogModelI(const LogModelI& other): order(other.order){

	}
	/**
	 * Assigment operator
	 *
	 * Ensures, that assigned model has the same order
	 */
	LogModelI& operator= (const LogModelI& other){
		assert(other.order == order);
		return *this;
	}


};



class NgramCounter : public BtreeMap{
public:
	int count_index;
    size_t order;
	NgramCounter(int order,int valuesize=1,int count_index=0):
		BtreeMap(order,valuesize*sizeof(double)),
		count_index(count_index),
		order(order)
	{
		assert(count_index == 0 || count_index == 1);

	};

	virtual ~NgramCounter(){};
	 double increase_count(int* key,double count){
		pair<bool,BtreeIterator> res = insert(key);
        double r = count;
		if (res.first){
			res.second.double_value()[count_index] = count;
		}
		else{
			res.second.double_value()[count_index] += count;
            r = res.second.double_value()[count_index];
		}
        return r;

	}



	 double get_count(int* key) const{
		 double result = 0;
		BtreeIterator it = find(key);
		if (it.has_value()){
			result = it.double_value()[count_index];
		}
		 return result;
	 }
	virtual void renormalize() {

	}

	virtual double get_prob(int* key) const{
		double result = -99;
		BtreeIterator it = find(key);
		if (it.has_value()){
			result = it.double_value()[0];
		}
		assert(result <= 0.0);
		return result;
	}
	virtual void calculate_prob(){
		double sumC = 0;
		double prob;
		int zeroCount = 0;
		BtreeIterator it = iterator();
		while(it.next()){
			sumC += it.double_value()[count_index];
			if (it.double_value()[count_index] == 0){
				zeroCount += 1;
                cout <<"zzzzzz zero count "<< it.key()[0] << endl;
            }
		}
		double d = 1.0;
		if (zeroCount > 0)
			d = 0.967;
		double sumProb = 0;
		it = iterator();
		while(it.next()){
			prob =  d * it.double_value()[count_index] / sumC;
			if (prob > 0){
				sumProb += prob;
				prob = log10(prob);
			}
			else{
				prob = log10((1.0-d) /(double) zeroCount);
			}
			it.double_value()[0] = prob;
			//it.double_value()[count_index] = 0;
		}

		cout << "sumProb " << sumProb << endl;
	}

		//virtual void learn_item(const ContextStateVector& sample){
		//	abort();
		//}

	void save_arpa(ostream& os,Vocab3& vocab){
		BtreeIterator it = iterator();
		while(it.next()){
			double c1 = it.double_value()[0];
			os << c1;
			os << '\t';
			int* key = it.key();
			os << vocab.Get(key[0]);
			if (order > 1){
				for (size_t i = 1; i < order; i++){
					os << " ";
					os << vocab.Get(key[i]);
				}
			}

			if (count_index > 0){
				double c2 = it.double_value()[count_index];
				if (c2!=0.0){
					os << '\t';
					os << c2;
				}
			}

			os << endl;

		}
	}

	void save_counts(ostream& os,Vocab3& vocab){
		BtreeIterator it = iterator();
		while(it.next()){
			os << vocab.Get(*it.key());
			for (size_t i = 1; i < order; i++){
				os << ' ';
				os << vocab.Get(it.key()[i]);
			}
			os << '\t';
			os << (int)it.double_value()[count_index];
			os << endl;
		}
	}
	bool load(const LString& line,Vocab3& vocab,bool add){
		//float sumProb = 0;
		Tokenizer tok(line,'\t');
		bool r = tok.next();
		if (!r)
			abort();
		float prob = 0;
		r = to_float(tok.token(),prob);
		assert(r);
		r = tok.next();
		assert(r);
		vector<int> wids;
		Tokenizer wtok(tok.token(),' ');
		while(wtok.next()){
			int id = -1;
			if (add){
				id = vocab.Add(wtok.token());
			}
			else{
				id = vocab.Find(wtok.token());
				assert(id >= 0);
			}
			wids.push_back(id);
		}
		assert(wids.size() == (size_t)order);

		assert(prob < 0.0);
		pair<bool,BtreeIterator> res = insert(wids.data());
		assert(res.first);
		res.second.double_value()[0] = prob;
		float backoff = 0.0;
		if (tok.next()){
			r = to_float(tok.token(),backoff);
			assert(count_index > 0);
			res.second.double_value()[count_index] = backoff;
			assert(r);
		}


		//cout << prob << endl;
		return true;
	}

	void set_zero(){
		BtreeIterator it = iterator();
		while(it.next()){
			it.double_value()[0] = 0;
		}
	}


	virtual void serialize(ostream& os){
		BtreeMap::serialize(os);
	}
	virtual void deserialize(istream& os) {
		BtreeMap::deserialize(os);
	}

};


class NgramBackoffCounter :public NgramCounter {
public:
	 NgramCounter* back;
NgramBackoffCounter(short order,short valnum,short count_index,NgramCounter* back):
	NgramCounter(order,valnum,count_index),
	back(back){
}

virtual ~NgramBackoffCounter(){

};

 double get_prob(int* key) const{
	double result = -99;
	BtreeIterator it = find(key);
	if (it.has_value()){
		return it.double_value()[0];
	}
	 // Backoff
	result = back->get_prob(key+1);
    BtreeIterator it2 =  back->find(key);
    if (it2.has_value()){
       result += it2.double_value()[1];
    }
	return result;
}


virtual void calculate_prob(){
	double d = 0.0;
	double c = 0.0;
	double prob = 0.0;
	double n1 = 0.001;
	double n2 = 0.001;
	double sumCount = 0;
	double sumProb = 0;
	int zeroCount = 0;

			// Calculate 1 and 2 frequencies
			//assert( this->size() > 0);
	BtreeIterator it = iterator();
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
	//	BtreeIterator hit = back->find(it.key());
      //  hit.double_value()[0] += 1;
        //cout <<  hit.double_value()[0] << endl;
	}
	assert(sumCount > 0.0);
			// Calculate b

	double b = n1 / (n1 + 2* n2);
	std::cout << "n1 " << n1 << " n2 " << n2 <<  " ZeroCount " << zeroCount << " b " << b << " Items " << size() <<std::endl;
	d = 0;
	double histCount = 0;
	it = iterator();
    assert(b > 0 && b < 1.0);
	while(it.next()){
		c =  it.double_value()[count_index];
		assert(c > 0);
	//assert(count > 0.0);
	//	d = (c - b) / c;
		// Context count
		BtreeIterator hit = back->find(it.key());
		assert(hit.has_value());
		histCount = hit.double_value()[back->count_index] ;
		assert(histCount > 0);
		assert(c <= histCount);
        d = 1;
       // d = 1 - b * hit.double_value()[0] /histCount;
        //cout << d << endl;
		prob = d * (c-b) /histCount;
		//cout << prob << " " << d << " " << c << " " << histCount  << " " << b << endl;
		assert(prob > 0.0 && prob <= 1.0);
		sumProb += prob;

		prob = log10(prob);
		//if(!isfinite(prob)){
		//	abort();
		//}
		it.double_value()[0] = prob;
		if (it.double_value()[count_index] > 0){
			it.double_value()[count_index] = 0;
		}
	}
    //cerr << "Problems " << problems << endl;
}

virtual void renormalize(){
	double prob;
	//double wordprob = 0;
	//double contextprob = 0;
	//double historyprob;

	int keybuf[30];
	BtreeIterator it = iterator();
	BtreeIterator bit = back->iterator();
	int badnum = 0;
	while(bit.next()){
		bit.double_value()[1] = 0;
		memcpy(keybuf,bit.key(),sizeof(int)* (order-1));
		keybuf[order-1] = -1;
		it = find_previous(keybuf);
		double num = 0;
		double denum = 0;
		while (it.next() && memcmp(keybuf,it.key(),sizeof(int)* (order-1)) == 0 ){
			prob = std::pow(10.0,it.double_value()[0]);
			num += prob;
			BtreeIterator it2 = back->find(it.key() + 1);
			if (it2.has_value()){
				denum += pow(10.0,it2.double_value()[0]);
			}
		}
		prob = log10((double)((1.0- num) / (1.0-denum)));
		if (isfinite(prob)){
			bit.double_value()[1] = prob;
		}
		else{
			cout << num << " " << denum << " " <<  prob << endl;
			badnum++;
			bit.double_value()[1] = 0;
            
		}
	}
		cout << "Normalized: badnum " << badnum  << endl;
		back->renormalize();
	}

};




#endif /* UNIGRAMS2_H_ */
