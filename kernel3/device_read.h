/* 
 * File:   device_write.h
 * Author: urb
 *
 * Created on October 19, 2018, 9:10 AM
 */

#ifndef DEVICE_READ_H
#define DEVICE_READ_H

ssize_t debug_read_operation(struct file *flip, char *buffer, size_t len, loff_t *offset);

#endif /* DEVICE_READ_H */

