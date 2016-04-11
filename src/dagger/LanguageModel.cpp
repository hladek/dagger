/*
 * BigramModel.cpp
 *
 *  Created on: 15.5.2013
 *      Author: voicegroup
 */

#include "LanguageModel.h"
//#include "Tokenizer.h"
#include <algorithm>



BigramModel::BigramModel(Vocab3& vocab) : LogModelI(3), vocab(vocab) {
	init();
}

void BigramModel::init()  {
    if (order == 4){
		ngrams.push_back(new NgramCounter(1,2,1));
		ngrams.push_back(new NgramBackoffCounter(2,2,1,ngrams[0]));
		ngrams.push_back(new NgramBackoffCounter(3,2,1,ngrams[1]));
        ngrams.push_back(new NgramBackoffCounter(4,1,0,ngrams[2]));
	}
    else if (order == 3){
		ngrams.push_back(new NgramCounter(1,2,1));
		ngrams.push_back(new NgramBackoffCounter(2,2,1,ngrams[0]));
		ngrams.push_back(new NgramBackoffCounter(3,1,0,ngrams[1]));
	}
	else if (order == 2){
		ngrams.push_back(new NgramCounter(1,2,1));
		ngrams.push_back(new NgramBackoffCounter(2,1,0,ngrams[0]));
	}
	else if (order == 1){
		ngrams.push_back(new NgramCounter(1,1,0));
	}

	//std::fill(wordbuffer,wordbuffer+900,-1);
}

void BigramModel::shutdown()  {
	int sz = ngrams.size();
	for (int i = 0; i < sz; i++){
		delete ngrams.back();
		ngrams.pop_back();
	}
}
void BigramModel::reset()  {
	shutdown();
	init();
}

BigramModel::~BigramModel() {
	shutdown();
}

//void BigramModel::calculate_prob(){
//	assert(order < 0);
//	for (size_t i = 0 ; i <order;i++){
//		ngrams[order -1 - i]->calculate_prob();
//	}
//	ngrams.back()->renormalize();
//}


double BigramModel::get_prob(int* key) const{
	return ngrams.back()->get_prob(key);
}

void BigramModel::load_arpa(const char* filename){
	int state = -1;
	bool res = false;
	ifstream fl(filename);
	LineTokenizer ft(fl);
	int sizes[4] = {0,0,0,0};
	//int wordids[3] = {0,0,0};
	//float prob = 0;
	//float backoff;
	while(ft.next()){
		// Zistenie sekcie
		if (ft.token().size() == 0){
			continue;
		}
		else if (ft.token() == "\\data\\"){
			state = 0;
			continue;
		}
		else if (ft.token().size() > 4 && ft.token()[0] == '\\' && ft.token()[ft.token().size()-1] == ':') {
				Tokenizer tok(LString(ft.token().start() + 1, ft.token().size()-1), '-');
				res = tok.next();
				if (!res)
					abort();
				res = to_int(tok.token(), state);
				assert(res);
				//cout << state << endl;
				res = tok.next();
				assert(res);
				continue;
		}
		else if (ft.token() == "\\end\\"){
			continue;
		}
		assert(state >= 0);

		// Citanie hlavicky
		if (state == 0) {
			//strtk::split(eq,line,std::back_inserter(result));
			Tokenizer tok(LString(ft.token().start() + 6, ft.token().size()-6), '=');
			res = tok.next();
			assert(res);
			int value = 0;
			res = to_int(tok.token(), value);
			assert(res);
			tok.next();
			//int size  = atoi(tok.startToken + tok.token()Size);
			int size = 0;
			res = to_int(tok.token(), size);
			assert(res);
			sizes[value-1] = size;
			if (value== 1){
				vocab.reserve(size*2);
			}

			//ngrams[value-1]->reserve(size);
		}else if (state > 0){
			// Citanie n gramov
			assert(sizes[state-1] > 0);
			if (sizes[state-1] > 0){
				//cout << state;
				//cout << " ";
				//size_t q = ngrams[state-1]->size();
				res = ngrams[state-1]->load(ft.token(),vocab,!externalVocabulary);
				assert(res);

				//assert(q == ngrams[state-1]->size() -1);
				//cout << state;
				//cout << ft.line << endl;
				//cout << sizes[state-1];
				//cout << endl;
				//cout << " --- ";
				//cout << ft.lineNumber;
				//cout << endl;
				sizes[state-1]--;
			}
		}
	}

	cerr << "Loaded ";
	cerr << ngrams[0]->size() << endl;

	cerr << "Loaded ";
	cerr << ngrams[1]->size();
	cerr << " bigrams" << endl;
	cerr << "Loaded ";
	cerr << ngrams[2]->size();
	cerr << " trigrams" << endl;
}

void BigramModel::save_arpa(const char* filename) {
	std::cout << "Saving ARPA .." << std::endl;
	ofstream outfile(filename);
	outfile << endl;
	outfile << "\\data\\" << endl;
	for (size_t i = 0 ; i < order; i++){
		outfile << "ngram ";
		outfile << (i+1);
		outfile << "=";
		outfile << ngrams[i]->size() << endl;
	}
	for (size_t i = 0 ; i < order; i++){
		outfile << endl;
		outfile << "\\";
		outfile << (i+1);
		outfile << "-grams:" << endl;
		ngrams[i]->save_arpa(outfile,vocab);
	}
	outfile << endl;
	outfile << "\\end\\" << endl;
}







void BigramModel::serialize(ostream& os) {
	char header[30] = "Lemon Ngram v1";
	os.write(header,strlen(header));
	os.write((char*)&order,2);
	for (size_t i = 0; i < order; i++){
		ngrams[i]->serialize(os);
	}
}

void BigramModel::deserialize(istream& os) {
	assert(order == 3);
	char header[30];
	os.read(header,14);
	// ngrams musi byt spravne inicializovane - pointery, indexy a tak
	os.read((char*)&order,2);
	for (size_t i = 0; i < order; i++){
		ngrams[i]->deserialize(os);
	}
}

