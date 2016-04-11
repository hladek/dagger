#ifndef UVPREFORK_H
#define UVPREFORK_H

#include <uv.h>
#include <vector>
#include <string>
#include <unistd.h>
#include <signal.h>

using namespace std;

class Annotator {
    public:

     virtual void annotate(char* line,size_t linesz,ostream& result){
         abort();
    }

};

class TCPForking
{
public:
    vector<char> readbuffer;
    size_t processedsz;
    size_t checkedsz;
    size_t is_idle;
    Annotator* anot;
    uv_tcp_t* reader;
    uv_timer_t *idle_checker;
    TCPForking(Annotator* anot){
        processedsz = 0;
        checkedsz = 0;
        is_idle = 0;
        this->anot = anot;
    }
    virtual void init(uv_stream_t* server){
        int pid = fork();
        if (pid == 0){
            uv_loop_t* loop = server->loop;
            idle_checker =  (uv_timer_t*)malloc(sizeof(uv_timer_t));
            uv_timer_init(loop,idle_checker);
            idle_checker->data = this;
            uv_timer_start(idle_checker, idle_check, 1000, 1000);
            reader = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
            uv_tcp_init(loop,reader);
            reader->data = this;
            int res = uv_accept(server,(uv_stream_t*)reader);
            if (res!=0){
                cerr << " Accept error" << endl;
                abort();
            }
            uv_read_start((uv_stream_t*)reader, before_read, on_read);
            uv_tcp_keepalive(reader,1,20);

            // Do not listen in child thread
            uv_close((uv_handle_t*)server,(uv_close_cb)free);
       }
    }
    static void on_close(uv_handle_t* handle){
        //delete handle->data;
        TCPForking *tr = (TCPForking*)handle->data;
        delete tr;
        free(handle);
        //uv_prepare_stop(handle);
        //uv_close((uv_handle_t*)handle,NULL);
        //delete handle;
    }
    virtual void shutdown(){
        // Koniec spojenia
       uv_timer_stop(idle_checker);
       idle_checker->data =NULL;
       uv_close((uv_handle_t*)idle_checker,(uv_close_cb)free);
        uv_read_stop((uv_stream_t*)reader);
        uv_close((uv_handle_t*)reader,on_close);
        cout << "Connection closed" << endl;
        //uv_prepare_t* cleanup= new uv_preparhis;e_t();
        //uv_prepare_init(reader.loop,cleanup);
        //cleanup->data = this;
        //uv_prepare_start(cleanup,on_close);
        //uv_idle_start
        //fprintf(stderr, "Listen error %s\n", uv_strerror(nread));

    }

    virtual ~TCPForking()
    {
    }
    void alloc(size_t suggested_size,uv_buf_t* buf){
         if (checkedsz + suggested_size > 256000){
            vector<char> newbuf(readbuffer.data() + processedsz,readbuffer.data() + checkedsz);
            readbuffer.swap(newbuf);
            processedsz = 0;
            checkedsz = readbuffer.size();
        }
        readbuffer.resize(checkedsz + suggested_size);
        buf->base = readbuffer.data() + checkedsz;
        buf->len = suggested_size;
    }
    virtual size_t process_chunk(char* data,ssize_t sz,uv_stream_t* stream){
        is_idle = 0;
        for (ssize_t i = 0; i < sz; i++){
            checkedsz += 1;
            if (data[i] == '\n'){
                //LString msg;
                //msg._start
                char* st = readbuffer.data() + processedsz;
                size_t sz = data + i - st + 1;
                processedsz += sz;
                stringstream ss;
                anot->annotate(st,sz -1,ss);
                ss << endl;
                string res = ss.str();
                send_buf(stream,res);
            }
        }
        return sz;
    }
    static void idle_check(uv_timer_t* handle){
        TCPForking *tr = (TCPForking*)handle->data;
        tr->is_idle += 1;
        if (tr->is_idle > 20){
           tr->shutdown();
        }
    }
    static void on_connection(uv_stream_t* server,int status)
    {
         if (status < 0) {
            fprintf(stderr, "New connection error %s\n", uv_strerror(status));
            abort();
        }
        cout << "Got connection" << endl;
        TCPForking* reader = new TCPForking((Annotator*)server->data);
        reader->init(server);
    }
     // Alokuje buffer na aktualnom readeri
    static void before_read(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf){
        TCPForking *tr = (TCPForking*)handle->data;
        tr->alloc(suggested_size,buf);
    }

     // Zapise buffer na aktualnom readeri a skusi ho spracovat
    static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf){
        TCPForking* thiss = (TCPForking*)stream->data;
        cout << "Reading " << nread << endl;
        if (nread < 0){
            thiss->shutdown();
            return;
        }

        thiss->process_chunk(buf->base,nread,stream);
    }
    static void send_buf(uv_stream_t* stream,string& strbuf) {
            if (uv_is_writable(stream)){
                uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
                string* s = new string();
                s->swap(strbuf);
                req->data = s;
                uv_buf_t uvbuf;
                uvbuf.base = (char*)s->data();
                uvbuf.len = s->size();
                uv_write(req,stream,&uvbuf,1,after_write);
            }
            else{
                cerr << "Write Error" << endl;
                abort();
            }
        }
 //Callback called after data was written on a stream. status will be 0 in case of success, < 0 otherwise.
        static void after_write(uv_write_t* req, int status){
             if (status != 0){
                 cerr << uv_strerror(status);
                 cerr << "after write error" << endl;
                //abort();
            }
            delete (string*)req->data;
            free(req);
            //uv_close((uv_handle_t*)req,(uv_close_cb)free);
        }
        // Zacne nacuvat na danom porte
        static uv_tcp_t* bind(uv_loop_t* loop,int port, Annotator* anot){
            signal(SIGPIPE,SIG_IGN);
            uv_tcp_t* listener = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
            listener->data = anot;
            uv_tcp_init(loop, listener);
            struct sockaddr_in addr;
            uv_ip4_addr("0.0.0.0", port, &addr);
            uv_tcp_bind(listener, (const struct sockaddr*)&addr, 0);
            int r = uv_listen((uv_stream_t*)listener, 10, on_connection);
            if (r) {
                fprintf(stderr, "Listen error %s\n", uv_strerror(r));
                abort();
            }
            return listener;
        }

        static int listen(int port,Annotator* anot){
            uv_loop_t* loop = uv_default_loop();
            bind(loop,port,anot);
            return uv_run(loop, UV_RUN_DEFAULT);
        }

 };

#endif // UVPREFORK_H
