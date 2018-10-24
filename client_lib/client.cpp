//#include <unistd.h>     // _SC_PAGE_SIZE
#include <stdio.h>      // perror
#include <stdlib.h>     // EXIT_SUCCESS
#include <string>       // memcpy(); std::string
#include <sys/stat.h>   // stat()
#include <unistd.h>     // _SC_PAGE_SIZE
#include <fcntl.h>      // O_CREAT|O_RDONLY|O_WRONLY

#include "ftransfer.h"


using namespace std;


#define BUF_SIZE 8192
int send(int argc, char* argv[]) {

    int input_fd, output_fd;    /* Input and output file descriptors */
    ssize_t ret_in, ret_out;    /* Number of bytes returned by read() and write() */
    char buffer[BUF_SIZE];      /* Character buffer */

    /* Are src and dest file name arguments missing */
    if(argc != 3){
        printf ("Usage: cp file1 file2");
        return 1;
    }

    /* Create input file descriptor */
    input_fd = open(argv [1], O_RDONLY);
    if (input_fd == -1) {
            perror ("open");
            return 2;
    }

    /* Create output file descriptor */
    output_fd = open(argv[2], O_WRONLY | O_CREAT, 0644);
    if(output_fd == -1){
        perror("open");
        return 3;
    }

    /* Copy process */
    while((ret_in = read (input_fd, &buffer, BUF_SIZE)) > 0){
            ret_out = write (output_fd, &buffer, static_cast<size_t>(ret_in));
            if(ret_out != ret_in){
                /* Write error */
                perror("write");
                return 4;
            }
    }

    /* Close file descriptors */
    close (input_fd);
    close (output_fd);

    return (EXIT_SUCCESS);
}


int queue_client(int argc, char* argv[])
{
//    char buffer[] = "this is string buffer!";
    char buffer[BUF_SIZE];      /* Character buffer */
    ssize_t size_f, ret_in, ret_out;

    if(argc == 1){
        printf ( "Stop server\n" );
        ftstop( );
        return (EXIT_SUCCESS);
    }
    if(argc != 2){
        printf ( "Usage: %s filename", argv[0] );
        return (EXIT_SUCCESS);
    }

    struct stat buf;
    string file_name = {argv[1]};

    if( stat( file_name.c_str(), &buf ) == 0 )
        size_f = buf.st_size;
    else
        return -1;

    /* Create input file descriptor */
    int input_fd = open (file_name.c_str(), O_RDONLY);
    if (input_fd == -1) { perror ("open"); return 2; }
    /* Create output file descriptor */
    int output_fd = ftopen( file_name.c_str(), O_WRONLY, static_cast<size_t>(size_f) );
    if (output_fd == -1) { perror ("open"); return 2; }

    // Copy process
    while( ( ret_in = read( input_fd, &buffer, BUF_SIZE ) ) > 0 ){
            ret_out = ftwrite( output_fd, buffer, static_cast<size_t>(ret_in) );
            if( ret_out != ret_in ) { perror( "write" ); return 4; }
    }

// take writed file from system to copy dir
    int copy_in_fd = ftopen(file_name.c_str(), O_RDONLY/*, static_cast<size_t>(size_f)*/);
    if (copy_in_fd == -1) { perror ( "open" ); return 2; }

    struct stat fstat;
    if( ftstat( copy_in_fd, &fstat ) != 0 ) { perror( "ftstat" ); return 4; }

    string copy_file_name;
    auto pos = file_name.rfind("/");
    if( pos == -1 )
    {
        copy_file_name = string{"./copy/"} + file_name;
    }
    else
    {
        copy_file_name = file_name.substr(0, pos) + string{"/copy"} + file_name.substr(pos);
    }
    int copy_out_fd = open(copy_file_name.c_str(), O_CREAT | O_WRONLY, 0644);
    if (copy_out_fd == -1) { perror ( "open" ); return 2; }

    // Copy process
    while( ( ret_in = ftread( copy_in_fd, buffer, BUF_SIZE ) ) > 0 ) {
            ret_out = write( copy_out_fd, &buffer, static_cast<size_t>(ret_in) );
            if( ret_out != ret_in ) { perror( "write" ); return 4; }
    }

    close (input_fd);
    close (copy_out_fd);
    if( ftclose(output_fd) < 0 ) { perror( "close|exit" ); return 4; }
    if( ftclose(copy_in_fd)   < 0 ) { perror( "close|exit" ); return 4; }

    return (EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    printf("Start...\n");
    queue_client(argc, argv);
	printf("Done...\n");
	return (EXIT_SUCCESS);
}




