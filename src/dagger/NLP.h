#ifndef DAGGER2_H
#define DAGGER2_H

#include "Observation.h"
#include "LanguageModel.h"
#include "UTokenizer.h"

class CorpusReader2 {
	public:
        virtual ~CorpusReader2(){};
	virtual bool next(){
		abort();
		return true;
	};
	virtual vector<LString> tokens() const{
		return vector<LString>();
	}
	virtual vector<int> states() const {
		return vector<int>();
	}
};

class HLineCorpusReader2 : public CorpusReader2 {
	LineTokenizer& lt;
	Vocab3& state_set;
	vector<LString> obs;
	int annotation_index;
	vector<int> statesvec;
    int startstate;
    string ss;
public:
	HLineCorpusReader2(LineTokenizer& lt,Vocab3& state_set,int annotation_index=1000):
	lt(lt),state_set(state_set),ss("<s>"){
		this->annotation_index = annotation_index;
	}
	virtual bool next(){
		if (lt.next()){
			if (lt.token().size() == 0){
				return next();
			}
			Tokenizer tok(lt.token(),' ');
			obs.clear();
            
			statesvec.clear();
            obs.push_back(LString(ss));
			statesvec.push_back(0);
             while(tok.next()){
				//cout << tok.token() << endl;
				
				Tokenizer tok2(tok.token(),'|');
				int i = 0;
				tok2.next();
				LString item = tok2.token();
				obs.push_back(item);
				while (tok2.next() && i != annotation_index){
					item = tok2.token();
					i += 1;
				}
				statesvec.push_back(state_set.Add(item));
			}
			obs.push_back(LString(ss));
			statesvec.push_back(0);
            obs.push_back(LString(ss));
			statesvec.push_back(0);
            if (statesvec.size() == 3)
				return next();
			assert (obs.size() > 0);
			return true;
		}
		return false;;
	};
	virtual vector<LString> tokens() const{
		return obs;
	}
	virtual vector<int> states() const {
		assert(statesvec.size() > 0);
		return statesvec;
	}
	
};

class BufferedCorpusReader : public CorpusReader2 {
	vector<int> corpus;
	Vocab3& obs;
	size_t j;
	vector<int> statesvec;
	vector<LString> observations;
	int id;
	public:
	BufferedCorpusReader(Vocab3& obs) : corpus(3),obs(obs){
		reset();
	}
	void add(const vector<LString>& tokens,const vector<int>& states){
		//cout << "add "<< tokens.size() << endl;
		for (size_t i = 0; i < tokens.size(); i++){
			corpus.push_back(j);
			corpus.push_back(obs.Add(tokens[i]));
			corpus.push_back(states[i]);
		}
		j += 1;
	}
	void reset(){
		j = 0;
		id = corpus[0];
	}
	virtual bool next(){
		statesvec.clear();
		observations.clear();
		//cout  << "next "<< id << " " << j << " " << corpus.size() <<  endl;
		id = corpus[j];
		while ( j < corpus.size() &&  corpus[j] == id){
			//cout  << "next state "<< id << " " << j << " " << corpus.size() <<  endl;
			id = corpus[j];
			j += 1;
			observations.push_back(obs.Get(corpus[j]));
			//cout << obs.Get(corpus[j]) << endl;
			j += 1;
			statesvec.push_back(corpus[j]);
			j += 1;
		}
		if (statesvec.size() == 0)
			return false;
		return true;;
	}
	virtual vector<LString> tokens() const{
		return observations;
	}
	virtual vector<int> states() const {
		return statesvec;
	}
	
};





typedef  vector<int>  Features;


class NLP {
public:
	virtual ~NLP(){};

    virtual Features prepare(const vector<LString>& words) {
		Features features;
		for (size_t i = 0; i < words.size(); i++) {
			// Mapovanie cisloviek
			LString word =words[i];
			int res = this->get_observation_set().Add(word);
            features.push_back(res);
        }
		return features;
	}
	
	virtual vector<int> predict(const Features& feats) {
		abort();
		return vector<int>();
	}
	
	virtual Vocab3& get_state_set()  =0 ;
    virtual Vocab3& get_observation_set()  =0;
	
	virtual void annotate_line(LString line,ostream& result){
        if (line.size() > 4 && ((line[0] == 'i' && line[1] == '\t') || (line == "--endtext"))){
            result << line << endl;
            return;
        }
		Tokenizer tok(line,' ');
		vector<LString> obs;
		vector<LString> annots;
		// Skip annotations after hline
		while (tok.next()){
			Tokenizer hl(tok.token(),'|');
			bool r = hl.next();
			if (r && hl.token().size() > 0 ){
				obs.push_back(hl.token());
				int wordsize = hl.token().size();
				LString anot;
				if (hl.next()){
					anot = LString(tok.token());
					anot = anot.lstrip(wordsize + 1);
				}
				annots.push_back(anot);
			
			}
		}
		if (obs.size() > 0){
			Features feats = prepare(obs);
			vector<int> res = predict(feats);
			assert(res.size() == obs.size());
			for (size_t i = 0; i < obs.size(); i++){
				LString word = obs[i];
				result << word;
				if (annots[i].size() > 0){
					result << "|";
					result << annots[i];
				}
				result << "|";
				LString state= get_state_set().Get(res[i]);
				result << state;
				result << " ";
			}
		}
	}
	int confusion(CorpusReader2& cr,NgramCounter& ng,bool skip_default,size_t startskip,size_t endskip){
        size_t oov = 0;
        size_t vocsize = get_observation_set().size();
		while (cr.next()){
            vector<LString> tokens = cr.tokens();
			Features feats = prepare(tokens);
			vector<int> res = predict(feats);
			vector<int> states = cr.states();
			
            for (size_t j = startskip; j < res.size()-endskip; j++) {
                int wid = get_observation_set().Add(tokens[j]);
                if (wid >= (int)vocsize)
                    oov += 1;
				int cl = states[j];
				if (!(skip_default && cl == 0)){
					int key[2] = {cl,res[j]};
					ng.increase_count(key,1);
				}
			}
		}
        return oov;
	}
	
	void print_confusion(const NgramCounter& ng){
		NgramCounter detect(1);
		NgramCounter match(1);
		BtreeIterator bi = ng.iterator();
		while (bi.next()){
			int i = bi.key()[0];
			int j = bi.key()[1];
			double c = bi.double_value()[0];
			detect.increase_count(&i,c);
			match.increase_count(&j,c);
		}
		cout << "Precision / Recall" << endl;
		bi = ng.iterator();
		while (bi.next()){
			int i = bi.key()[0];
			int j = bi.key()[1];
			if (i == j){
				double c = bi.double_value()[0];
				double d = detect.get_count(&i);
				double m = match.get_count(&i);
				cout << get_state_set().Get(i) << "\t" <<c/d << "\t" << c/m << "\t" << c <<"/" << d << endl;
			}
		}
	}

        void print_accuracy(const NgramCounter& ng){
            int good = 0;
            int bad = 0;
            BtreeIterator bi = ng.iterator();
	    while (bi.next()){
			int i = bi.key()[0];
			int j = bi.key()[1];
			int c = (int)bi.double_value()[0];
			if (i == j){
				good += c;
			}
			else {
				bad += c;
			}
		} 
        cout << good << " " << bad << endl;
	    cout << "Err" << (double)bad / (double)(good + bad) << endl;

        }
	
	virtual void learn_corpus(CorpusReader2& cr)=0;
	virtual void reset() =0;
	
	void evaluate(CorpusReader2& cr,bool skip_default){
		NgramCounter ng(2);
		size_t oov = confusion(cr,ng,skip_default,1,2);
        //print_confusion(ng);
		print_accuracy(ng);
        cout << "OOV:" << oov << endl;
	}
	
	void tenfold(CorpusReader2& cr){
		Vocab3 obs;
		BufferedCorpusReader master(obs);
		while (cr.next()){
			master.add(cr.tokens(),cr.states());
		}
		NgramCounter ng(2);
		
		for (size_t i = 0; i < 10; i++){
			BufferedCorpusReader trainset(obs);
			BufferedCorpusReader testset(obs);
			master.reset();
			size_t j = 0;
			while (master.next()){
				if (j % 10 == i){
					testset.add(master.tokens(),master.states());
				}
				else{
					trainset.add(master.tokens(),master.states());
				}
				j += 1;
			}
			reset();
			learn_corpus(trainset);
		
			confusion(testset,ng,false,1,2);
			NgramCounter ng2(2);
			testset.reset();
			confusion(testset,ng2,false,1,2);
			cout << "----------------------" << endl;
			print_confusion(ng2);
			cout << "----------------------" << endl;
			
		
		}
		print_confusion(ng);
	}
	
	
};




#endif // DAGGER2_H
