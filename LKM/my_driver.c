#include<linux/init.h>
#include<linux/module.h>
#include<linux/fs.h>
#include<linux/kernel.h>
#include<linux/types.h>
#include<linux/uaccess.h>
#include<linux/errno.h>
#include<linux/unistd.h>

// PROTOTYPES
int my_driver_init(void);
void my_driver_exit(void);
int my_open(struct inode*, struct file*);
int my_release(struct inode*, struct file*);
ssize_t my_read (struct file*, char* , size_t, loff_t*);
ssize_t my_write(struct file*, const char *, size_t , loff_t*);
loff_t my_llseek(struct file*, loff_t, int);


// GLOBAL DEVICE BUFFER
static char dev_buffer[1024]; // 1024 bytes

// FILE OPERATIONS STRUCT
struct file_operations f_ops = 
{
	.owner = THIS_MODULE,
	.llseek = my_llseek,
	.read = my_read,
	.write = my_write,
	.open = my_open,
	.release = my_release
};

// IMPLEMENTATIONS
int my_driver_init(void)
{
	// args : MAJOR_NUM, DEVICE, &f_ops
	int ret_val = register_chrdev(240, "simple_character_device", &f_ops);
	// Register the dev at load
	if (ret_val == 0) { 	
		printk(KERN_ALERT "Successfully registered 'simple_character_device'\n");
		return 0; // success
	} 
	printk(KERN_ALERT "Failed to register simple_character_device\n");
	return ret_val; // failure : -EINVAL (specified num is not valid), -EBUSY (specified major num is busy)
}

loff_t my_llseek(struct file* f_ptr, loff_t offset, int whence) {
	loff_t cursor;

	printk(KERN_ALERT "whence %d\n", whence);

	// Where to start
	if (whence == SEEK_SET) { 
		printk(KERN_ALERT "SEEK SET");
		cursor = offset;
	} else if(whence == SEEK_CUR) {
		printk(KERN_ALERT "SEEK CUR");
		cursor = f_ptr -> f_pos + offset;
	} else if (whence == SEEK_END) {
		printk(KERN_ALERT "SEEK END");
		cursor = 1024 + offset;
	} 

	if (cursor < 0) {
		return -EINVAL; // adding offset made cursor become negative
	} else if (cursor > 1024) {
		cursor = 1024; // position must be inside buffer
	}

	f_ptr -> f_pos = cursor; // Update file position 
	printk(KERN_ALERT "New file position: %lld\n", cursor);
	return cursor;
}

ssize_t my_read(struct file* f_ptr, char* user_buffer, size_t count, loff_t *offp) {
	// Read device contents from kernel to user space

	int bytes_to_read;
	int bytes_not_read;
	int buffer_capacity = 1024 - *offp;

	printk(KERN_ALERT "%ld bytes requested\n", count);

	if(buffer_capacity == 0){
		printk(KERN_ALERT "EOF has reached. No data was read\n");
		return 0; 
	} else if(buffer_capacity >= count) {
		bytes_to_read = count;
		printk(KERN_ALERT "%d bytes to read\n", bytes_to_read);
	} else {
		bytes_to_read = buffer_capacity;
		printk(KERN_ALERT "%d bytes to read\n", bytes_to_read);
	}

	bytes_not_read = copy_to_user(user_buffer, dev_buffer+*offp, bytes_to_read);

	if (bytes_not_read  < 0) {
		printk(KERN_ALERT "Error copying to user\n");
		return -EFAULT; 
	}

	// Update offset
	*offp += (bytes_to_read - bytes_not_read);

	printk(KERN_ALERT "Successfully read %d bytes\n", (bytes_to_read - bytes_not_read));
	return bytes_to_read - bytes_not_read; // number of bytes successfully read
}

ssize_t my_write(struct file *f_ptr, const char *user_buffer, size_t count, loff_t *offp) {
	// Write contents from user to kernel space

	int bytes_to_write;
	int bytes_not_written;
	int buffer_capacity = 1024 - *offp;

	printk(KERN_ALERT "%ld bytes requested\n", count);

	if(buffer_capacity == 0) {
		printk(KERN_ALERT "Error: Device buffer is full\n");
		return -1; // attempt to write beyond buffer
	} else if(buffer_capacity >= count) {
		bytes_to_write = count;
		printk(KERN_ALERT "%d bytes to write\n", bytes_to_write);
	} else {
		bytes_to_write = buffer_capacity;
		printk(KERN_ALERT "%d bytes to write\n", bytes_to_write);
	}

	bytes_not_written=copy_from_user(dev_buffer+*offp, user_buffer, bytes_to_write);

	if(bytes_not_written < 0){
		printk(KERN_ALERT "Error copying from user");
		return -EFAULT;
	}

	// Update offset
	*offp += (bytes_to_write - bytes_not_written);

	printk(KERN_ALERT "Successfully wrote %d bytes\n", (bytes_to_write - bytes_not_written));
	return bytes_to_write - bytes_not_written;
}


int my_open(struct inode* inode, struct file* f_ptr) {
	printk(KERN_ALERT "Successfully opened 'simple_character_device'\n");
	return 0; // success
}

int my_release(struct inode* inode, struct file* f_ptr) {
	printk(KERN_ALERT "Successfully closed 'simple_character_device'\n");
	return 0; // success
}

void my_driver_exit(void)
{
	unregister_chrdev(240, "simple_character_device");
	printk(KERN_ALERT "Successfully removed 'simple_character_device'");
}

module_init(my_driver_init);
module_exit(my_driver_exit);

