#include <unistd.h>     // ftruncate()
#include <mqueue.h>
#include <sys/mman.h>   // shm_open
#include <assert.h>
#include <string.h>     // memcpy();
#include <map>

#include "ftransfer.h"
#include "../common/cwrdefs.h"


using namespace std;


struct fd_info
{
    struct message      msg;
    int                 shm;
    void                *shm_ptr;
    struct file_info    fi;
    int                 oflag;
    mqd_t               mq_cmd;
    mqd_t               mq_resp;
    size_t              offset;
    bool                writing = false;
    bool                reading = false;
};

// TODO: make map thread safe by tamplate safe_ptr and map->extract()
static map<int, struct fd_info> fd_map;


size_t make_hash(const char *buf)
{
    return hash<string>{}(string(buf));
}


int get_new_fd()
{
    static int fd_count = 0;
    return fd_count++;
}


void mq_send_wo_resp(const mqd_t &mq_cmd, const int64_t &msg_type, const ssize_t &msg_len)
{// send the message with command and size
    struct message msg;
    msg.msg_type = msg_type;
    msg.msg_len = msg_len;
    char *ch_msg = reinterpret_cast<char*>(&msg);
    cout << "CLIENT: Send message:" << endl
         << "message type: " << dec << msg.msg_type << endl
         << "message length: " << dec << msg.msg_len << endl;
    assert( mq_send(mq_cmd, ch_msg, message_length, 0) != -1);
}


ssize_t mq_send_w_resp(const mqd_t &mq_cmd, const mqd_t &mq_resp, const int64_t &msg_type, const ssize_t &msg_len)
{
    mq_send_wo_resp(move(mq_cmd), move(msg_type), move(msg_len));
    struct message msg;
    char *ch_msg = reinterpret_cast<char*>(&msg);
    assert( mq_receive(mq_resp, ch_msg,  message_length, reinterpret_cast<unsigned int *>(0)) != 1 );
    cout << "CLIENT: Received message:" << endl
         << "message type: " << dec << msg.msg_type << endl
         << "message length: " << dec << msg.msg_len << endl;
    // check the response
    assert( msg.msg_type == MSGT_OK );//&& msg.msg_len == ERR_NOERR );
    return msg.msg_len;
}


//int stat (const char *__file, struct stat *__restrict __buf)
//{
//    return -1;
//}


int ftopen(const char *__file, int __oflag, size_t __size, ...)
{

    struct fd_info fd;
    fd.fi.hash      = make_hash(__file);
    fd.fi.offset    = 0;
    fd.fi.size      = __size; /*TODO - get file size from DB*/
    fd.fi.length    = __size;
    fd.offset       = 0;
    fd.oflag        = __oflag;
    if( __oflag & O_RDWR )
        // must be set to O_RDONLY or O_WRONLY
        return -1;

    // open the mail queue
    // to send commands:
    fd.mq_cmd = mq_open(MQ_CMD_THR_00, O_WRONLY);
    if( fd.mq_cmd<0 )
    {
        cout << "error open message queue: " << dec << fd.mq_cmd << endl;
        return (EXIT_FAILURE);
    }
    // to recieve responses:
    fd.mq_resp = mq_open(MQ_RESP_THR_00, O_RDONLY);
    if( fd.mq_resp<0 )
    {
        cout << "error open message queue: " << dec << fd.mq_cmd << endl;
        return (EXIT_FAILURE);
    }

    // create shared memory
    fd.shm = shm_open(SHM_THR_00, O_CREAT|O_RDWR, S_IRWXO|S_IRWXG|S_IRWXU);
    if ( fd.shm == -1 )
    {//assert( fd.shm != -1 );
        perror("shm_open");
        return -1;
    }
    assert( ftruncate (fd.shm, fi_size ) != -1 );
    fd.shm_ptr = mmap( nullptr, fi_size, PROT_WRITE|PROT_READ, MAP_SHARED, fd.shm, static_cast<__off_t>(0) );
    assert( fd.shm_ptr != reinterpret_cast<void *>(-1) );

    // prepare shared memory to send file info
    // copy file info to shared memory
    *static_cast<struct file_info*>( fd.shm_ptr ) = fd.fi;
    cout << "CLIENT: Prepared message:" << endl;
    print_info(fd.fi);

    mq_send_w_resp(fd.mq_cmd, fd.mq_resp, (fd.oflag & O_WRONLY) ? MSGT_WRITE : MSGT_READ, fi_size);
//SERVER?
    //assert( munmap( fd.shm_ptr, fd.msg.msg_len ) == 0);

    auto ret = fd_map.emplace(get_new_fd(),move(fd));
    if( ret.second == false )
        return (-1);
    return ret.first->first;
}


size_t ftlseek( int __fd, size_t __offset, int __whence )
{
//s    int i[] = {SEEK_SET,SEEK_CUR,SEEK_END};
    auto it = fd_map.find(__fd);
    if( it == fd_map.end() )
        return static_cast<size_t>(-1);
    auto &fd = it->second;
    switch (__whence) {
    case SEEK_SET:
    {
        if( __offset > fd.fi.size)
            return static_cast<size_t>(-1);
        return fd.offset = __offset;
    }
    case SEEK_CUR:
    {
        if( fd.offset + __offset > fd.fi.size)
            return static_cast<size_t>(-1);
        return fd.offset += __offset;
    }
    case SEEK_END:
    {
        if( fd.fi.size + __offset > fd.fi.size)
            return static_cast<size_t>(-1);
        return fd.fi.size += __offset;
    }
    }//switch

    return static_cast<size_t>(-1);
}


int ftstat(const int __fd, struct stat *__buf)
{
    auto it = fd_map.find( __fd );
    if( it == fd_map.end() )
        return -1;
    auto &fd = it->second;

    assert( ftruncate( fd.shm, fi_size ) != -1 );
    fd.shm_ptr = mmap( nullptr, fi_size, PROT_WRITE|PROT_READ, MAP_SHARED, fd.shm, static_cast<__off_t>(0) );
    assert( fd.shm_ptr != reinterpret_cast<void *>(-1) );

    auto st_size = mq_send_w_resp(fd.mq_cmd, fd.mq_resp, MSGT_GET_LEN, fi_size);
    __buf->st_size = static_cast<__off_t>(fd.fi.length = fd.fi.size = static_cast<size_t>(st_size));

    auto m_st_mtime = mq_send_w_resp(fd.mq_cmd, fd.mq_resp, MSGT_GET_TIME, fi_size);
    __buf->st_mtime = static_cast<__time_t>(m_st_mtime);

    return 0;
}


size_t ftwrite(int __fd, const void *__buf, size_t __n)
{
    auto it = fd_map.find(__fd);
    if( it == fd_map.end() )
        return static_cast<size_t>(-1);
    auto &fd = it->second;

    /*TODO: check fd.oflag*/

    if( fd.writing == false )
    {// prepare write to shm
        assert( ftruncate (fd.shm, static_cast<__off_t>(fd.fi.length) ) != -1 );
        fd.shm_ptr = mmap( nullptr, fd.fi.length, PROT_WRITE|PROT_READ, MAP_SHARED, fd.shm, 0 );
        assert( fd.shm_ptr != reinterpret_cast<void*>(-1) );
        fd.writing = true;
    }
    // check that shm buffer fulled
    if( fd.offset == fd.fi.size )
        return 0;
    // truncate __buff to available size
    size_t len = ((fd.offset + __n) > fd.fi.size) ? (fd.fi.size - fd.offset) : (__n);

    memcpy( static_cast<char*>( fd.shm_ptr ) + fd.offset, __buf, len );
    fd.offset += len;
    //len truncated!
    //assert( fd.offset <= fd.fi.size );

    //todo: write flush() function
    if( fd.offset == fd.fi.size )
    {// all data writed - send file
        mq_send_w_resp(fd.mq_cmd, fd.mq_resp, MSGT_WR_FILE, static_cast<ssize_t>(fd.offset));
    }
    return len;
}


size_t ftread(int __fd, void *__buf, size_t __nbytes)
{
    auto it = fd_map.find(__fd);
    if( it == fd_map.end() )
        return static_cast<size_t>(-1);
    auto &fd = it->second;

    if( (fd.fi.length - fd.offset) <= 0 )
        return 0;
    // reduce __nbytes to available size
    size_t len = ((fd.offset + __nbytes) > fd.fi.length) ? (fd.fi.length - fd.offset) : (__nbytes);
    /*TODO: check fd.oflag*/

    if( fd.reading == false )
    {// shm is empty yet
        assert( ftruncate (fd.shm, static_cast<__off_t>(fd.fi.length) ) != -1 );
        fd.shm_ptr = mmap( nullptr, fd.fi.length, PROT_WRITE|PROT_READ, MAP_SHARED, fd.shm, 0 );
        assert( fd.shm_ptr != reinterpret_cast<void*>(-1) );

        mq_send_w_resp(fd.mq_cmd, fd.mq_resp, MSGT_RD_FILE, static_cast<ssize_t>(fd.fi.length));

        fd.reading = true;
    }
    memcpy( __buf, static_cast<char*>( fd.shm_ptr ) + fd.offset, len );
    fd.offset += len;
    //len truncated!
    //assert( fd.offset <= fd.fi.size );
    return len;
}


int ftclose (int __fd)
{
    auto it = fd_map.find(__fd);
    if( it == fd_map.end() )
        return -1;
    auto &fd = it->second;

    cout << "CLIENT: close message queues" << endl;
    assert( mq_close(fd.mq_cmd) != -1 );
    assert( mq_close(fd.mq_resp) != -1 );
    cout << "CLIENT: close shared memory" << endl;
    shm_unlink( SHM_THR_00 );
    return ( EXIT_SUCCESS );
}


int ftstop( )
{
    fd_info fd;
    fd.mq_cmd = mq_open(MQ_CMD_THR_00, O_WRONLY);
    if( fd.mq_cmd<0 )
    {
        cout << "error open message queue: " << dec << fd.mq_cmd << endl;
        return (EXIT_FAILURE);
    }
    fd.mq_resp = mq_open(MQ_RESP_THR_00, O_RDONLY);
    if( fd.mq_resp<0 )
    {
        cout << "error open message queue: " << dec << fd.mq_cmd << endl;
        return (EXIT_FAILURE);
    }

    mq_send_wo_resp(fd.mq_cmd, MSGT_STOP, fi_size);

    return (EXIT_SUCCESS);
}
