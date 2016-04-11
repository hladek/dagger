#ifndef _SPLITTER
#define _SPLITTER
#include "Vocab3.h"
#include <set>
#include "Tokenizer.h"
#include <vector>
#include "UTokenizer.h"
#include "Ngrams.h"
#include <deque>
#include <map>

using namespace std;

class Splitter {

		Vocab3 voc;
public:
	NgramCounter class_map;
    const size_t min_suffix_sz;
    Vocab3 class_set;
    Vocab3& observation_set;
    bool reversed;
	
    Splitter(Vocab3& observation_set,size_t minsufsz,bool reversed) : 
        class_map(2,1),
        min_suffix_sz(minsufsz),
    observation_set(observation_set)
    {
		class_set.Add(LString("-/")); // Zero suffix
        this->reversed = reversed;

	}
    
void serialize(ostream& os){
		class_set.serialize(os);
		class_map.serialize(os);
	}
	
	void deserialize(istream& os){
		class_set.deserialize(os);
		class_map.deserialize(os);
	}

        // Ku slovu vrati optimalne rozdelenie a zoznam slov
        // ktore patria k rozdeleniu
	pair<LString, vector<int> > findsplit(int wid,set<size_t>& unsplit2){
		LString w = voc.Get(wid);
		UTokenizer utok(w);
		vector<int> br;
        LString beststem = w;
        deque<size_t> tokens;
		while(utok.next()){
            tokens.push_back(utok.token().size());
		}
        LString stem = w;
        stem = stem.rstrip((tokens.back()));
        tokens.pop_back();
        stem = stem.rstrip((tokens.back()));
        tokens.pop_back();
        //stem = stem.rstrip((tokens.back()));
        //tokens.pop_back();
        while (tokens.size() > 7){
            stem = stem.rstrip((tokens.back()));
            tokens.pop_back();
        }
        while (tokens.size() >= min_suffix_sz ) {
            stem = stem.rstrip((tokens.back()));
            tokens.pop_back();
            vector<int> res;
		    for (set<size_t>::iterator it = unsplit2.begin(); it != unsplit2.end(); ++it){
                int wd = *it;
                if (voc.Get(wd).starts_with(stem) ){
                      res.push_back(wd);
                }
                else if (res.size() > 0){
                    break;
                }
			}
            if (res.size() > 1){
                br =res; 
                beststem = stem;
                break;
            }
        }
        if (br.empty() ){
          br.push_back(wid);
        }
        // POZOR, vysledok nemusi byt najdeny
		//cout << "--" << bi << endl;
		//cout << "--" << br.size() << endl;
		//cout << "--" << w.size() << endl;


		return make_pair(beststem,br);
	}

    size_t sencode(int i){
        size_t iid = 1000 - voc.Get(i).size();
        iid <<= 32;  
        iid += i;
        return iid;
    }
    void go3(){
        map<string,int> suffs;
        map<string,int> stems;
        map<string,set<string> > signatures;
        for (size_t i = 0; i < voc.size(); i++){
            LString rword = voc.Get(i);
            string word = to_reversed(rword);
            LString lw(word);
		    UTokenizer utok(lw);
            int j = 0;
            int k = 0;
            while (utok.next()){
                j += utok.token().size(); 
                if (k > 0 &&  j  < (int)word.size()){
                    string suff = word.substr(j,(int)word.size() -j);
                    string stem = word.substr(0,j);
                    suffs[suff] += 1;
                    stems[stem] += 1;
                    signatures[stem].insert(suff);
                }
                k += 1;
            }
        }
        for (map<string, set<string> >::iterator it = signatures.begin(); it != signatures.end(); ++it ) {
            bool isgood = true;
            for (set<string>::iterator it2 = it->second.begin(); it2 != it->second.end() ; it2++){
                int thr = stems[it->first];
                if (suffs[*it2] <= thr || suffs[*it2] < 10){
                    isgood = false;
                    break;
                }
            }
            if (isgood){
                for (set<string>::iterator it2 = it->second.begin(); it2 != it->second.end() ; it2++){
                    class_set.Add(LString(*it2));
                    //cout << *it2 << endl;
                }

            }


        }

    }
    void go2(){
        for (size_t i = 0; i < voc.size(); i++){
            LString rword = voc.Get(i);
            string words = to_reversed(rword);
            LString word(words);
            word = word.lstrip(2);
            for (size_t i = 2 ; i < word.size(); i++){
                class_set.Add(word);
                word = word.lstrip(1);
            }
        }

    }
   	
    void go(){
        set<size_t> unsplit;
        set<size_t> unsplit2;

        assert(unsplit.size() == 0);
        voc = voc.sort();
        for (size_t i = 0; i < voc.size(); i++){
            unsplit.insert(sencode(i));
            unsplit2.insert(i);
        }

        while (unsplit.size() > 0){
            size_t unsplitfirst = *unsplit.begin();
            size_t m = unsplitfirst & 0xffffffff;
		    //unsplit.erase(unsplitfirst);
            //unsplit2.erase(m);
            LString spec = voc.Get(m);

            //cout << "<<<<<<<<<<" << spec << endl;
		    pair<LString, vector<int> > rem = findsplit(m,unsplit2);
            LString stem = rem.first;
            assert(stem.size() > 0);
            if (rem.second.size() > 0){
                LString suff = stem;
                string suffs;
                if (reversed){
                    suffs = to_reversed(stem);
                    suff = LString(suffs);
                }
                class_set.Add(suff); //cout << stem << endl;
                for (size_t i = 0 ; i < rem.second.size(); i++){
                    unsplit.erase(sencode(rem.second[i]));
                    unsplit2.erase(rem.second[i]);
                }
            }
        }
        cerr << class_set.size() << " classes " << endl;
    }
    void load_vocab(const char* filename){
	        ifstream ifs(filename);
		LineTokenizer lt(ifs);
		while (lt.next()){
			Tokenizer tok(lt.token(),' ');
			while (tok.next()){
				Tokenizer tok2(tok.token(),'|');
				tok2.next();
				LString word = tok2.token();
                // To ake dlhe slova vyberieme ma velky vplyv na vyslednmu presnost
				if (word.size()  > 3 && is_lower(word) && word.size() < 17 ){
                    string words;
                    if (reversed) {
                        words = to_reversed(word);
                        word = LString(words);
                    }
                    voc.Add(word);
				}
			}
		}
     }
    virtual int get_class(int wid){
        LString word = observation_set.Get(wid);
        int suffid = -1;
        //int suffid = -1;
        int key[2] = {wid,-2};
        // Hladanie pravilda pre word-class (suffix)
        BtreeIterator it = this->class_map.find_previous(key);
        if (it.next() && it.key()[0] == wid){
            suffid =  it.key()[1];
        }
        else  
        if (is_alpha(word) &&  word.size() > 3 && word.size() < 40) {
            LString suf = word;
            if (reversed){
                suf = suf.lstrip(1);
            }
            else {
                suf = suf.rstrip(1);
            }
            for (size_t j = 0; j < word.size()-1; j++){
                if (reversed){
                    suf = suf.lstrip(1);
                }
                else {
                    suf = suf.rstrip(1);
                }
                int r = this->class_set.Find(suf);
                if (r >= 0){
                    suffid = r;
                    key[1] = suffid;
                    class_map.increase_count(key,1);
                    break;
                }
            }
            
        }
       //if (is_title(word)){
       //     suffid += 1000000;
       //}

        return suffid;

    }

    
};

#endif
