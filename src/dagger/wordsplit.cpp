/*
 * wordsplit.cpp
 *
 * Split each word in a dived dictionary in unsupervised way into stem and suffix
 *
 *  Created on: Oct 27, 2014
 *      Author: dano
 */
#include "Splitter.h"
#include "Command.h"


int main(int argc, char **argv) {

    Vocab3 vocab;
    Splitter s(vocab,3,true);
    s.load_vocab(argv[1]);
    s.go();
    NgramCounter unigrams(1);
    ifstream ifs1(argv[1]);
    LineTokenizer lt1(ifs1);
    while(lt1.next()){
        Tokenizer tok(lt1.token(),' ');
        while(tok.next()){
            int wid = vocab.Add(tok.token());
            unigrams.increase_count(&wid,1);
        }
    }

    ifstream ifs(argv[1]);
    LineTokenizer lt(ifs);
    NgramCounter ng(2);
    while(lt.next()){
        Tokenizer tok(lt.token(),' ');
        while(tok.next()){
            int wid = vocab.Add(tok.token());
            int clid = s.get_class(wid);
            if (clid < 0 || unigrams.get_count(&wid) > 100 ){
                clid = wid;
            }
            else {
                string cl = "-" + s.class_set.Get(clid).str();
                clid = vocab.Add(LString(cl));
            }

            int key[2] = {clid,wid};
            ng.increase_count(key,1);
            cout << vocab.Get(clid) << " "; 
        }
        cout << endl;
    }
    BtreeIterator it = ng.iterator();
    ofstream ofs("my.classes");
    while(it.next()){
         ofs << vocab.Get(it.key()[1]) << "\t" << vocab.Get(it.key()[0]) << "\t" << (int)it.double_value()[0] << endl;
    }
    ofstream ofs2("my.classmap");
    it = s.class_map.iterator();
    while (it.next()){

         ofs2 << vocab.Get(it.key()[0]) << "\t" << s.class_set.Get(it.key()[1]) << endl;
    }
}


