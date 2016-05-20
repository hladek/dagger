#include "NLP.h"
#include "Command.h"
#include "Splitter.h"
#include "UTokenizer.h"
#include "Viterbi3.h"
#include "TCPFork.h"



class TrigramCorpusReader : public CorpusReader2 {
LineTokenizer& lt;
    Vocab3& state_set;
    vector<LString> obs;
    vector<int> statesvec;
    int c;
public:
    TrigramCorpusReader(LineTokenizer& lt,Vocab3& state_set):
    lt(lt),state_set(state_set){
        c = 0;
    }
    virtual bool next(){
        if (c > 0){
            c -= 1;
            return true;
        }
        if (lt.next()){
            Tokenizer tokl(lt.token(),'\t');
            obs.clear();
            statesvec.clear();
            tokl.next();
            Tokenizer tok(tokl.token(),' ');
            while(tok.next()){
                Tokenizer tok2(tok.token(),'|');
                tok2.next();
                obs.push_back(tok2.token());
                tok2.next();
                statesvec.push_back(state_set.Add(tok2.token()));
            }
            tokl.next();
            to_int(tokl.token(),c);
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

class DaggerPOS : public NLP
                  , public Annotator
{
public:
    Vocab3 observation_set;
    Vocab3 state_set;
    int numid;
    int orderid;

    Observation observation_model;
    Observation class_observation_model;
        BigramModel transition_model;
        Splitter splitter;
   // vector<int> words;
    DaggerPOS():
        // Pocita pravdepodobnost slova
       observation_model(2,0,1),
       // Pocita pravdepodobnost stavu na zaklade koncovky
       // ????
       class_observation_model(2,1,0),
       transition_model(state_set),
       splitter(observation_set,2,true)
       // transition_model(0,2,0.001)
    {
        state_set.Add(LString("%"));
        numid = observation_set.Add(LString("NUMBER"));
        orderid = observation_set.Add(LString("ORDER"));

    }
    virtual void annotate(char* st,size_t sz,ostream& result){
        LString line(st,sz);
        annotate_chunk(line,result);
    }
    virtual Vocab3& get_state_set() {
        return state_set;
    }

    virtual Vocab3& get_observation_set()  {
        return observation_set;
    }

    void serialize(ostream& os){
        observation_set.serialize(os);
        state_set.serialize(os);
        observation_model.serialize(os);
        class_observation_model.serialize(os);
        splitter.serialize(os);
        transition_model.serialize(os);
    }

    void deserialize(istream& os){
        observation_set.deserialize(os);
        state_set.deserialize(os);
        observation_model.deserialize(os);
        class_observation_model.deserialize(os);
        splitter.deserialize(os);
        transition_model.deserialize(os);
    }

    virtual void reset(){
        observation_model.clear();
        class_observation_model.clear();
        transition_model.reset();
    }

    virtual vector<int> predict(const Features& features)   {
        Viterbi3 * vit = NULL;
        int key[3] = {-1,-1,-1};
        assert(features.size() > 0);
           for (size_t i = 0; i < features.size(); i++) {
            BtreeIterator it = observation_model.iterator();
            key[0] = features[i];
            key[1] = -1;
            vit = new Viterbi3(transition_model,vit);
            it = observation_model.find_previous(key);
            while(it.next() && it.key()[0] == key[0]){
                vit->add_state(it.key()[1],it.double_value()[0]);
            }
            if (vit->states.size() == 0) {
                LString word = observation_set.Get(features[i]);
                int classid = splitter.get_class(features[i]);

                if (i < 2 && is_title(word)){
                    string lw = to_lower(word);
                    int lwid = observation_set.Add(LString(lw));
                    key[0] = lwid;
                    it = observation_model.find_previous(key);
                    while(it.next() && it.key()[0] == key[0]){
                        double v = it.double_value()[0];
                        v -= transition_model.ngrams[0]->get_prob(it.key() +1);
                        vit->add_state(it.key()[1],v);
                    }      //cout << observation_set.Get(features[i]) << endl;
                }
                if ( vit->states.size() == 0) {
                    key[0] = classid;
                    key[1] = -1;
                    it = class_observation_model.find_previous(key);
                                     //cout << word << endl;
                    //cout << "--" << class_set.Get(key[0]) << endl;
                    //cout << observation_set.Get(features[i+1]) << endl;
                    while(it.next() && it.key()[0] == key[0]){
                        double v = it.double_value()[0];

                        v -= transition_model.ngrams[0]->get_prob(it.key() +1);
                         vit->add_state(it.key()[1], v);
                        //cout << state_set.Get(it.key()[1]) << " " <<v  << endl;
                    }
                }
               if (vit->states.size() == 0){
                    vit->add_state(0,-1);
                        //cout << word << endl;
                }

            }
        }
        vector<int> res = vit->backtrack();
        delete vit;
        return res;
    }

    virtual void learn_corpus(CorpusReader2& cr){
        ADLearner stle(transition_model);
        int k[4] =  {-1,-1,-1,-1};
        while (cr.next()){
            //
            vector<LString> words = cr.tokens();
            Features features = prepare(words);
            vector<int> states = cr.states();
            transition_model.learn_item(states.data(),states.size());
            for (size_t i = 0 ; i < features.size(); i++){
                k[0] = features[i];
                k[1] = states[i];
                observation_model.increase_count(k,1);
           }
        }
        BtreeIterator it = observation_model.iterator();
        while (it.next()){
            double val = it.double_value()[0];

            if (val < 5){
                k[0] = splitter.get_class(it.key()[0]);
                if (k[0] >= 0) {
                    k[1] = it.key()[1];
                    class_observation_model.increase_count(k,val );
                }
            }
        }
           stle.process();
        observation_model.calculate_prob();
        class_observation_model.calculate_prob();
        cout <<  observation_model.size() << " word observation rules" << endl;
        cout <<  class_observation_model.size() << " class observation rules" << endl;
        cout << state_set.size() << " tags" << endl;
        cout << observation_set.size() << " words" << endl;
    }


       virtual Features prepare(const vector<LString>& words) {
        Features features;
        for (size_t i = 0; i < words.size(); i++) {
            // Mapovanie cisloviek
            LString word =words[i];
            int res = this->observation_set.Add(word);
            if (is_digit(word)){
                res = numid;
            }
            else if (word[word.size() -1] == '.' &&  is_digit(word.rstrip(1))){
                res = orderid;
            }
            features.push_back(res);
        }
        return features;
    }
    virtual ~DaggerPOS(){};


    void load_lexicon(LineTokenizer& lt) {
        int sz = observation_set.size();
        while (lt.next()){
            int key[2] = {-1,-1};
            Tokenizer tok(lt.token(),'\t');
            tok.next();
            LString word = tok.token();
            vector<LString> it;
            it.push_back(word);
            Features f = prepare(it);
            tok.next();
            LString cl = tok.token();
            key[0] = f[0];
            key[1] = state_set.Add(cl);
            //cout << "Added " << word << " " <<key[0] << " " <<  cl << " " << key[1] << endl;
            observation_model.increase_count(key,1);
            key[0] = f[1];
            if (key[0] >= 0)
                class_observation_model.increase_count(key,1);
        }
        cerr << "Loaded " << observation_set.size() -sz << " lexicon forms " << endl;
    }
};

int main(int argc, char **argv) {
    LCParser  lp;
    Arg porta(lp,"-p",Arg::INTEGER, "Run server on port" ,"--port");
    Arg modela(lp,"-m",Arg::STRING, "Set model file" ,"--model");
//    Arg mapa(lp,"-a",Arg::BOOL, "Map number to NUMBER and map NAME" ,"--map");

    Arg traigrama(lp,"-r",Arg::STRING, "Train model on trigram counts" ,"--train-trigrams");
    Arg corpusa(lp,"-c",Arg::STRING, "Train model on given corpus file" ,"--train-corpus");
    Arg evala(lp,"-e",Arg::STRING, "Evaluate model on given corpus file" ,"--evaluate");
    Arg indexa(lp,"-i",Arg::INTEGER, "Annotation index from corpus file" ,"--index");
    Arg classa(lp,"-w",Arg::STRING, "Load word-class mapping" ,"--class");


    Arg filea(lp,"-f",Arg::STRING, "Tag given file" ,"--file");
    Arg lexicona(lp,"-l",Arg::STRING, "Use additional lexicon for learning" ,"--lexicon");
    Arg verbosea(lp,"-v",Arg::BOOL, "Print additional info" ,"--verbose");

    Arg helpa(lp,"-h",Arg::BOOL, "Print help" ,"--help");
    //Arg loada(lp,"-l",Arg::STRING, "Load binary model" ,"--load");
    lp.parse(argc,argv);
    if (argc < 3){
        cout << "Dagger Morphological Analysis v3" << endl;
        cout << "Daniel Hladek (c) 2015 Technical University of Kosice" << endl;
        lp.help(cout);
    }
    DaggerPOS dag;
    if (lexicona.isValid){
        ifstream ifs(lexicona.arg.start());
        LineTokenizer lt(ifs);
        dag.load_lexicon(lt);
    }
    int annotation_index = 1000;
    if (indexa.isValid){
        annotation_index = (int)indexa.value;
    }
    if (verbosea.isValid){
        //dag.verbose = true;
    }
    if (corpusa.isValid || traigrama.isValid ){
        // Ucenie
        if (corpusa.isValid){
            dag.splitter.load_vocab(corpusa.arg.start());
            dag.splitter.go3();
            int k[2] = {dag.numid,dag.splitter.class_set.Add(LString("NUMCLASS"))};
            dag.splitter.class_map.increase_count(k,1);
            k[0] = dag.orderid;
            k[1] = dag.splitter.class_set.Add(LString("ORDERCLASS"));
            dag.splitter.class_map.increase_count(k,1);
            ifstream ifs(corpusa.arg.str().c_str());
            LineTokenizer lt(ifs);
            HLineCorpusReader2 hl(lt,dag.state_set,annotation_index);
            dag.learn_corpus(hl);
            //dag.transition_model.save_arpa("states.lm");
        }
        else if (traigrama.isValid){
             ifstream ifs(traigrama.arg.str().c_str());
            LineTokenizer lt(ifs);
            TrigramCorpusReader hl(lt,dag.state_set);
            dag.learn_corpus(hl);
        }
        if (modela.isValid){
            ofstream ofs(modela.arg.start());
            dag.serialize(ofs);
        }
    }
    else if(modela.isValid){
        ifstream ifs(modela.arg.start());
        dag.deserialize(ifs);
    }
    if (dag.observation_set.size() < 3){
        cout << "No model" << endl;
    }
    // Evaluacia
    if (evala.isValid ){
        ifstream ifs(evala.arg.str().c_str());
        LineTokenizer lt(ifs);
        HLineCorpusReader2 hl(lt,dag.state_set,annotation_index);
        dag.evaluate(hl,false);
    }
    // Klasifikacia
    // zo suboru
    if (filea.isValid ){
        ifstream ifs(filea.arg.start());
        LineTokenizer lt(ifs);
        while(lt.next()){
            dag.annotate_chunk(lt.token(),cout);
            cout << endl;
        }
    }
    else if (porta.isValid){
        return TCPFork::start(&dag,porta.value);
    }
    // Klasifikacia zo standardneho vstupu
    else if (!evala.isValid && !corpusa.isValid) {
        LineTokenizer lt(cin);
        while(lt.next()){
            dag.annotate_chunk(lt.token(),cout);
            cout << endl;
        }
    }
}
