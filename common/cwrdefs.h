#ifndef CWRDEFS
#define CWRDEFS
#include <vector>
#include <stdio.h>
#include <iostream>     // cout
#include <iomanip>      // setfill() setw()

#define MQ_CMD_THR_00   "/mq_req_thr_00"
#define MQ_RESP_THR_00  "/mq_resp_thr_00"
#define SHM_THR_00      "shm_thr_00"
using namespace std;

#define MSGT_STOP   (-777)
#define MSGT_RESET  (-77)
#define MSGT_ERROR  (-1)
#define MSGT_OK     (0)

#define MSGT_READ     (10)
#define MSGT_RD_FILE  (11)

#define MSGT_WRITE    (20)
#define MSGT_WR_FILE  (21)

#define MSGT_GET_LEN  (30)
#define MSGT_GET_TIME (31)

//errors
#define ERR_NOERR       (0)
#define ERR_UNKNOWN_CMD (1)
#define ERR_LENGTH      (2)
#define ERR_WRONG_CMD   (3)
#define ERR_ERROR_LEN   (4)
#define ERR_SHM_OPEN    (5)
#define ERR_CREATE_FILE (6)
#define ERR_READ_FILE   (7)
#define ERR_WRITE_FILE  (8)


struct file_info{
    size_t      hash;
    size_t      size;       // file size
    size_t      offset;     // offset from the beginning of the file from which the buffer starts
    size_t      length;     // buffer length
};
constexpr auto fi_size = sizeof (file_info);


struct message
{
    int64_t     msg_type;
    ssize_t      msg_len;
};
constexpr auto message_length = sizeof (struct message);


//DEBUG START
void print_info(struct file_info &fi)
{
    cout << "---- hash:   0x" << hex << setfill('0') << setw(16) << fi.hash <<  endl
         << "---- size:   "   << dec << fi.size    << endl
         << "---- offset: "   <<        fi.offset  << endl
         << "---- length: "   <<        fi.length  << endl;
}
//DEBUG END


#endif //CWRDEFS
