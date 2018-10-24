//#include <fcntl.h>
#include <assert.h>
#include <mqueue.h>
#include <stdlib.h>     // EXIT_SUCCESS
#include <sys/mman.h>   // shm_open
#include <sys/stat.h>
#include <string.h>     // strlen(); memcpy();

#include <unistd.h>     // TEMP - read()/write()

// TODO: later - #include <unistd.h>   // _SC_PAGE_SIZE
#include <iostream>     // DEBUG


// TODO: make serialize memory in mq and shm
// TODO: recieve message from queue with timeout

#include "../common/cwrdefs.h"


using namespace std;


void recieve_cmd(void)
{
    ;
}


mqd_t init_mq(const char *mq_name, int oflag)
{
    //  init message queue attributes
    struct mq_attr attr;
    attr.mq_flags   = 0;
    attr.mq_maxmsg  = 10;
    attr.mq_msgsize = message_length;
    attr.mq_curmsgs = 0;
    //todo: edit access rights
    mqd_t mq = mq_open( mq_name, O_CREAT | oflag, 0666, &attr );
    assert( mq != -1 );
    return mq;
}


int init_shm(const char *shm_name)
{
    int shm = shm_open(shm_name, O_RDWR, S_IRWXO|S_IRWXG|S_IRWXU);
    //assert( shm != -1 );
    return shm;
}


pair<int64_t, size_t> recv_cmd( mqd_t mq )
{
    struct message msg;
    char *ch_msg = reinterpret_cast<char*>(&msg);
    assert( mq_receive(mq, ch_msg,  message_length, reinterpret_cast<unsigned int *>(0)) != -1 );
    cout << "SERVER: Received message:" << endl
         << "message type: " << dec << msg.msg_type << endl
         << "message length: " << dec << msg.msg_len << endl;
    return {msg.msg_type, msg.msg_len};
}


//todo: return Maybe
struct file_info shm_get_fi(const int shm)
{
    struct file_info fi;

    void* shm_ptr = mmap( nullptr, fi_size, PROT_WRITE|PROT_READ, MAP_SHARED, shm, 0 );
    assert( shm_ptr != reinterpret_cast<void*>(-1) );
    fi = *static_cast<struct file_info*>(shm_ptr);
    print_info(fi);

    return fi;
}


void send_resp( const mqd_t &mq, int64_t msg_type, ssize_t msg_len )
{
    struct message msg;
    char *ch_msg = reinterpret_cast<char*>(&msg);
    msg.msg_type = msg_type;
    msg.msg_len  = msg_len;
    cout << "SERVER: Send RESPONSE:" << endl
         << "message type: " << dec << msg.msg_type << endl
         << "message length: " << dec << msg.msg_len << endl;
    assert( mq_send( mq, ch_msg, message_length, 0 ) != -1 );
}


void send_resp_ok( const mqd_t mq )
{
    send_resp( mq, MSGT_OK, ERR_NOERR );
}


void send_resp_err( const mqd_t mq, const ssize_t err_num )
{
    send_resp( mq, MSGT_ERROR, err_num );
}


int write_file(const void *shm_ptr, const struct file_info fi)
{
    if( fi.length > 0 )
    {
        /* Create output file descriptor */
        auto filename = to_string(fi.hash) + string(".dmp");
        auto output_fd = open(filename.c_str(), O_WRONLY | O_CREAT, 0644);
        if( output_fd == -1 )
        {
            cout << "SERVER: error create file: " << filename << endl;
            perror("open");
            return (ERR_WRITE_FILE);
        }
        assert( write( output_fd, shm_ptr, fi.length ) == static_cast<ssize_t>(fi.length) );
        close(output_fd);
    }
    return (EXIT_SUCCESS);
}


int read_file(void *shm_ptr, const struct file_info fi)
{
    if( fi.length > 0 )
    {
        /* Create input file descriptor */
        auto filename = to_string( fi.hash ) + string(".dmp");
        auto input_fd = open( filename.c_str(), O_RDONLY );
        if( input_fd == -1 )
        {
            cout << "SERVER: error create file: " << filename << endl;
            perror("open");
            return (ERR_READ_FILE);
        }
        vector<char> buffer(fi.length);
//        assert(
                    read( input_fd, buffer.data(), fi.length )
//                    == 0 )
                ;
        memcpy( shm_ptr, buffer.data(), fi.length );
        close(input_fd);
    }
    return (EXIT_SUCCESS);
}


ssize_t read_flength(struct file_info &fi)
{
    struct stat fstat;
    /* Create input file descriptor */
    auto filename = to_string( fi.hash ) + string(".dmp");
    stat(filename.c_str(), &fstat);
    fi.length = static_cast<size_t>(fstat.st_size);
    return fstat.st_size;
}


ssize_t read_ftime(struct file_info fi)
{
    struct stat fstat;
    /* Create input file descriptor */
    auto filename = to_string( fi.hash ) + string(".dmp");
    stat(filename.c_str(), &fstat);
    return fstat.st_mtime;
}


void print_msg(pair<int64_t, size_t> msg)
{
    cout << "command: " << dec << msg.first << endl;
    cout << "length:  " << dec << msg.second << endl;
}


int queue_server(const char *name)
{
    //bool shm_inited = false;
    int shm;
    struct file_info fi;
    pair<int64_t, size_t> msg;
    string m_name_cmd = {string{"/mq_req_"} + string{name}};
    string m_name_resp = {string{"/mq_resp_"} + string{name}};

    mqd_t mq_cmd = init_mq( m_name_cmd.c_str(),  O_RDONLY);
    mqd_t mq_resp= init_mq( m_name_resp.c_str(),  O_WRONLY);

    do
    {
        //auto [msg_type,msg_len] = recv_cmd( mq );
        // todo: replace auto msg to auto&[cmd,len]
        msg = recv_cmd( mq_cmd );
        switch ( msg.first )
        {
        case MSGT_READ:
        case MSGT_WRITE:
        {
            if( 1/*shm_inited == false*/ ) // client unlink shm
            {
                shm = init_shm( (string{"shm_"} + string{name}).c_str() );
                if( shm == -1 )
                {
                    send_resp_err( mq_resp, ERR_SHM_OPEN);
                    break;
                }
                //shm_inited = true;
            }
            fi = shm_get_fi(shm);
            send_resp_ok( mq_resp );

//            do{
//                msg = recv_cmd( mq_cmd );
//                if(    msg.first == MSGT_STOP
//                    || msg.first == MSGT_RESET )
//                    break;
//                else if( msg.first == MSGT_WR_FILE || msg.first == MSGT_RD_FILE )
//                {
//                    if( msg.second != fi.size )
//                    {
//                        send_resp_err( mq_resp, ERR_ERROR_LEN );
//                        continue;
//                    }
//                    else
//                        break;
//                }
//                else if( msg.first == MSGT_GET_LEN || msg.first == MSGT_GET_TIME )
//                    break;
//                send_resp_err( mq_resp, ERR_WRONG_CMD );
//            }while(1);
//            // if exit
//            if( msg.first == MSGT_STOP || msg.first == MSGT_RESET )
//                break;

//            void* shm_ptr = mmap( nullptr, fi.length, PROT_WRITE|PROT_READ, MAP_SHARED, shm, 0 );
//            assert( shm_ptr != reinterpret_cast<void*>(-1));

//            int res = EXIT_SUCCESS;
//            switch (msg.first) {
//            case MSGT_WR_FILE:
//                res = write_file( shm_ptr, fi );
//                send_resp( mq_resp, res == EXIT_SUCCESS ? MSGT_OK : MSGT_ERROR, res );
//                break;
//            case MSGT_RD_FILE:
//                res = read_file( shm_ptr, fi );
//                send_resp( mq_resp, res == EXIT_SUCCESS ? MSGT_OK : MSGT_ERROR, res );
//                break;
//            case MSGT_GET_LEN:
//            {
//                ssize_t fsize = read_flength( fi );
//                send_resp( mq_resp,  == EXIT_SUCCESS ? MSGT_OK : MSGT_ERROR, fsize );
//                break;
//            }
//            case MSGT_GET_TIME:
//                ssize_t ftime = read_ftime( fi );
//                send_resp( mq_resp,  == EXIT_SUCCESS ? MSGT_OK : MSGT_ERROR, ftime );
//                break;
//            }
            break;
        }
        case MSGT_WR_FILE:
        {
            void* shm_ptr = mmap( nullptr, fi.length, PROT_WRITE|PROT_READ, MAP_SHARED, shm, 0 );
            assert( shm_ptr != reinterpret_cast<void*>(-1));
            auto res = write_file( shm_ptr, fi );
            send_resp( mq_resp, res == EXIT_SUCCESS ? MSGT_OK : MSGT_ERROR, res );
            break;
        }
        case MSGT_RD_FILE:
        {
            void* shm_ptr = mmap( nullptr, fi.length, PROT_WRITE|PROT_READ, MAP_SHARED, shm, 0 );
            assert( shm_ptr != reinterpret_cast<void*>(-1));
            auto res = read_file( shm_ptr, fi );
            send_resp( mq_resp, res == EXIT_SUCCESS ? MSGT_OK : MSGT_ERROR, res );
            break;
        }
        case MSGT_GET_LEN:
        {
            ssize_t fsize = read_flength( fi );
            send_resp( mq_resp, fsize < 0 ? MSGT_ERROR : MSGT_OK , fsize );
            break;
        }
        case MSGT_GET_TIME:
        {
            ssize_t ftime = read_ftime( fi );
            send_resp( mq_resp, ftime < 0 ? MSGT_ERROR : MSGT_OK, ftime );
            break;
        }
        case MSGT_STOP:
        case MSGT_RESET:
            //send_resp_ok( mq_resp );
            break;

        default:
            send_resp( mq_resp, MSGT_ERROR, ERR_UNKNOWN_CMD );
        }
    }while( msg.first != MSGT_STOP );

    /* cleanup */
    cout << "SERVER: cleanup" << endl;
    mq_close(mq_cmd);
    mq_close(mq_resp);
    mq_unlink(m_name_cmd.c_str());
    mq_unlink(m_name_resp.c_str());

    return (EXIT_SUCCESS);
}


static struct mq_attr attr;
static struct file_info fi;
static struct message msg;
static char *ch_msg = reinterpret_cast<char*>(&msg);


int test_server(int argc, char *argv[])
{
    /* initialize the queue attributes */
    attr.mq_flags   = 0;
    attr.mq_maxmsg  = 10;
    attr.mq_msgsize = message_length;
    attr.mq_curmsgs = 0;

    /* create the message queue */
    mqd_t mq = mq_open(MQ_CMD_THR_00, O_CREAT | /*O_WRONLY*/O_RDWR /* | O_NONBLOCK */, 0666, &attr);
    assert( mq != -1 );

    assert( mq_receive(mq, ch_msg,  message_length, reinterpret_cast<unsigned int *>(0)) != -1 );
    // check message type
    // todo: enums commands
    assert( msg.msg_type == MSGT_WRITE );

    cout << "SERVER: Received message:" << endl
         << "message type: " << dec << msg.msg_type << endl
         << "message length: " << dec << msg.msg_len << endl;

    int shm = shm_open(SHM_THR_00, O_RDWR, S_IRWXO|S_IRWXG|S_IRWXU);
    assert( shm != -1 );
    void* shm_ptr = mmap( nullptr, /*fi_size*/msg.msg_len, PROT_WRITE|PROT_READ, MAP_SHARED, shm, 0 );
    assert( shm_ptr != reinterpret_cast<void*>(-1) );
    fi = *static_cast<struct file_info*>(shm_ptr);
    print_info(fi);

    // response OK
    msg.msg_type = 0;   // type - RESPONSE
    msg.msg_len = 0;    // errorno - OK
    cout << "SERVER: Send message:" << endl
         << "message type: " << dec << msg.msg_type << endl
         << "message length: " << dec << msg.msg_len << endl;
    assert( mq_send( mq, ch_msg, message_length, 0 ) != -1 );
    //client unmapped?!
    //assert( munmap(shm_ptr, msg.msg_len) == 0);

    // receive file!
    assert( mq_receive(mq, ch_msg,  message_length, reinterpret_cast<unsigned int *>(0)) != -1 );
    // check message type
    // todo: enums commands
    assert( msg.msg_type == MSGT_WR_FILE );

    cout << "SERVER: Received message:" << endl
         << "message type: " << dec << msg.msg_type << endl
         << "message length: " << dec << msg.msg_len << endl;

    if( msg.msg_len > 0 )
    {
        shm_ptr = mmap( nullptr, msg.msg_len, PROT_WRITE|PROT_READ, MAP_SHARED, shm, 0 );
#if 0
        cout << "SERVER: Received file:" << endl
             << (char *)shm_ptr << endl;
#else
        /* Create output file descriptor */
        auto filename = argv[1];
        auto output_fd = open(filename, O_WRONLY | O_CREAT, 0644);
        if( output_fd == -1 )
        {
            perror("open");
            return 3;
        }
        auto b = ( write( output_fd, shm_ptr, msg.msg_len ) == static_cast<ssize_t>(msg.msg_len) );
        assert( b );
        close(output_fd);
#endif
        //client unmapped?!
        //assert( munmap(shm_ptr, msg.msg_len) == 0);
    }
    else
        cout << "SERVER: Received file empty" << endl;

    // response OK
    msg.msg_type = 0;   // type - RESPONSE
    msg.msg_len = 0;    // errorno - OK
    cout << "SERVER: Send message:" << endl
         << "message type: " << dec << msg.msg_type << endl
         << "message length: " << dec << msg.msg_len << endl;
    assert( mq_send( mq, ch_msg, message_length, 0 ) != -1 );

    /* cleanup */
    cout << "SERVER: cleanup" << endl;
    mq_close(mq);
    mq_unlink(MQ_CMD_THR_00);

    return (EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
	printf("Start...\n");
#if 0
    test_server( argc, argv );
#else
    queue_server( "thr_00" );
#endif
	printf("Done...\n");
	return (EXIT_SUCCESS);
}
