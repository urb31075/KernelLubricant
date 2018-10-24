/* 
 * File:   device_write.h
 * Author: urb
 *
 * Created on October 19, 2018, 9:10 AM
 */

#ifndef DEVICE_READ_H
#define DEVICE_READ_H

ssize_t get_nvdimm_stub_data_amount(void);
ssize_t read_nvdimm_stub(char *buffer);


#endif /* DEVICE_READ_H */

