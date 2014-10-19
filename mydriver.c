#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/io.h>

#define PORT1 0x3F8
#define PORT2 0x2F8

//IF DLAB
#define Latch_Low 0x00
#define Latch_High 0x01
//ELSE
#define RW_Buffer 0x00
#define Interupt 0x01

#define Latch_Low_Max 0x00
#define Latch_High_Max 0x01

#define LCR 0x03

#define DLAB 0b10000000
#define Bit8 0b00000011
#define No_Parity 0b00000000
#define Stop_Bit1 0b00000000

static int major = 0; module_param(major, int, 0);
static int nb_dev = 1; module_param(nb_dev, int, 0);

struct mydriver_dev {
	struct cdev cdev;
} mydriver_dev;

/*
 * Operation
 */
static int mydriver_open(struct inode *inode, struct file *filp) {
	printk("file opened\n");
	return 0;
}

static ssize_t mydriver_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
	char byte;

	int i = 0;
	while (i < count) {
		byte = inb(PORT2 + RW_Buffer);
		copy_to_user(buf + i, &byte, 1);
		++i;
	}
	printk("file read\n");
	return count;
}

static ssize_t mydriver_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
	char byte;
	int i = 0;
	while (i < count) {
		copy_from_user(&byte, buf + i, 1);
		outb(byte, PORT2 + RW_Buffer);
		++i;
	}
	printk("file written\n");
	return count;
}

static int mydriver_release(struct inode *inode, struct file *file) {
	printk("file released\n");
	return 0;
}

struct file_operations mydriver_fops = {
	.owner = THIS_MODULE,
	.read = mydriver_read,
	.write = mydriver_write,
	.open = mydriver_open,
	.release = mydriver_release
};

/*
 * Module Initialisation
 */
static void setup_cdev(struct cdev *m_dev, int minor, struct file_operations *m_fops)
{
	int err, dev = MKDEV(major, minor);

	cdev_init(m_dev, m_fops);
	m_dev->owner = THIS_MODULE;
	m_dev->ops = m_fops;
	err = cdev_add(m_dev, dev, 1);
	if (err)
		printk(KERN_NOTICE "Error adding cdev: %d\n", err);
}

static int allocate_dev_number(void) {
	int dev, result;

	if (major) {
		dev = MKDEV(major, 0);
		result = register_chrdev_region(dev, nb_dev, "mydriver");
	} else {
		result = alloc_chrdev_region(&dev, 0, nb_dev, "mydriver");
		major = MAJOR(dev);
	}
	if (result < 0) {
		printk(KERN_WARNING "mydriver: can't get major %d\n", major);
	}
	return result;
}

static void setup_port(int port) {
	outb(0, port + Interupt); /* Turn off interrupts - Port1 */
	
	outb(DLAB, port + LCR); /* SET DLAB ON */
	outb(Latch_Low_Max, port + Latch_Low);
	outb(Latch_High_Max, port + Latch_High);
	outb(Bit8 |No_Parity | Stop_Bit1, port + 3); /* AND SET DLAB OFF*/
	
	outb(0x01, PORT1 + Interupt); /* Turn interruptes back on */
}

static int __init mydriver_init(void)
{	
	allocate_dev_number();
	setup_cdev(&mydriver_dev.cdev, 0, &mydriver_fops);
	setup_port(PORT2);
	printk("mydriver: init with major=%d nb_dev=%d\n", major, nb_dev);
	return 0;
}

static void __exit mydriver_exit(void)
{
	int dev = MKDEV(major, 0);
	cdev_del(&mydriver_dev.cdev);
	unregister_chrdev_region(dev, nb_dev);
	printk("mydriver: exit\n");
	return;
}

module_init(mydriver_init);
module_exit(mydriver_exit);

MODULE_LICENSE("GPL");
