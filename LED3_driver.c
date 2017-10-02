#include <linux/init.h>
#include <linux/gpio.h> 
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/module.h>

#define ANTAL_DEVICES 2
MODULE_LICENSE("GPL");
MODULE_AUTHOR ("Joachim Hejslet");
MODULE_DESCRIPTION("read/Write from/to LED");

static struct cdev my_cdev;
struct file_operations my_fops;
static dev_t devno;

// init funktion
static int __init led3_driver_init(void)
{
    int err = 0; // vi ligger fejl i denne variabel
    printk(KERN_ALERT "LED3 init\n"); // printer til kernel ved opstart
    
    err = gpio_request(21, "LED3"); //  anmoder om gpio port og angiver label
    if (err < 0) // tjekker for fejl - hvis ja gøres understående
        {
            printk (KERN_ALERT "fej1 1: registrering\n"); 
            goto err_exit;
        }
    
    gpio_direction_output(21,1); // sætter gpio port 21 retning som output og default værdi = 1 (LED3 er tændt ved start)
    devno = MKDEV(64, 0); // major , minor

    register_chrdev_region(devno, ANTAL_DEVICES ,"LED3"); // 
    
    cdev_init(&my_cdev, &my_fops); // initialiserer c-dev
    
        
    err = cdev_add(&my_cdev, devno, ANTAL_DEVICES); 
    if (err < 0) 
        {
            printk (KERN_ALERT "fejl 2: cdev add\n");
            goto err_free_buf;
        }
    
    printk (KERN_ALERT "Init done\n");
    return 0;    // succes
    

    err_free_buf: 
        gpio_free(21);  // frigør porten og afregistrerer
		unregister_chrdev_region(devno,ANTAL_DEVICES);    

    err_exit:
    return err;
}

// exit funktion 
static void __exit led3_driver_exit(void)
{
    printk(KERN_ALERT "LED3 exit\n");
    
    cdev_del(&my_cdev); // fjerner my_cdev
    
    unregister_chrdev_region(devno, ANTAL_DEVICES); // afregistrerer enhed
    
    gpio_free(21); // sætter gpio porten fri
       
    printk(KERN_ALERT "Exit done\n");

}


// open funktion 
int mygpio_open(struct inode *inode, struct file *filep)
{
    int major, minor;
    major = MAJOR(inode->i_rdev);
    minor = MINOR(inode->i_rdev);
    printk("Opening MyGpio Device [major], [minor]: %i, %i\n", major, minor);
    return 0;
}

// close funktion
 int mygpio_release(struct inode *inode, struct file *filep)
{
    int minor, major;

    major = MAJOR(inode->i_rdev);
    minor = MINOR(inode->i_rdev);
    printk("Closing/Releasing MyGpio Device [major], [minor]: %i, %i\n", major, minor);

    return 0;
}

// read funktion
 ssize_t my_led_read(struct file *filep, char __user *buf, size_t count, loff_t *f_pos)
 {
	 // init af variabler
    int err; 
    char value[3];
    int read = gpio_get_value(21); // læser værdi og ligger den i read
    
    int len = sprintf(value,"%d",read); // konverterer fra int til char (value) 
    ++len;
    
    len = len > count ? count : len;
    err = copy_to_user(buf, value, len);  // kopierer til user space
        if (err != 0)
            printk("error ved ctu");
            
    *f_pos += len;
    return len;    
 }
 
 
 // write funktion
 ssize_t my_led_write(struct file *filep, const char __user *ubuf,size_t count, loff_t *f_pos)
 {
	 // init variabler
    char myBuf[5];
    int value;
    int err;
    int len = 1;
    
    len = len > count ? count : len;
    err = copy_from_user(myBuf, ubuf, len); // kopierer fra user space
        if (err != 0)
            printk("Error ved CFU");
    
    
    sscanf(myBuf, "%i", &value); // konverterer til char fra int 
    printk("LED value modtaget: %i", value); // printer value til Kernel
    
	// vi bruger if til at sætte gpio port værdi alt efter hvilken værdi value har 
    if (value == 0)
        gpio_set_value(21, 0); // slukker LED
    else if (value == 1)
        gpio_set_value(21, 1); // tænder LED
        
    *f_pos += count;
    return len;
 }

 
 // struct til funktionerne 
struct file_operations my_fops = {
.read = my_led_read,
.write = my_led_write,
.open = mygpio_open,
.release = mygpio_release};

module_init(led3_driver_init); // funktions kald ved module init 
module_exit(led3_driver_exit); // funktions kald ved module exit
