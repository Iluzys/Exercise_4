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
MODULE_DESCRIPTION("read Write from Switch");

static struct cdev my_cdev;
struct file_operations my_fops;
static dev_t devno;

// init funktion, kaldes ved init (insmod af driver) 
static int __init sw2_driver_init(void)
{
    int err = 0;
    printk(KERN_ALERT "SW2 init\n"); // udksriver til kernel at init er igang
    
	// vi anmoder om  SW2's GPIO port
    err = gpio_request(16, "SW2"); // gpio pin , navn (label)
    if (err < 0) 
        {
            printk (KERN_ALERT "fej1 1: registrering\n"); // udskriver hvis der opstod fej1
            goto err_exit; // hvis der var en fejl går vi til err_exit
        }
    
    gpio_direction_input(16); // sætter gpio nr  (16) retning som input
    devno = MKDEV(65, 0); 	  // giver major og minor nummer

    register_chrdev_region(devno, ANTAL_DEVICES ,"SW2"); //  
    
    cdev_init(&my_cdev, &my_fops); // initialiserer c-dev
    
        
    err = cdev_add(&my_cdev, devno, ANTAL_DEVICES); 
    if (err < 0) 
        {
            printk (KERN_ALERT "fejl 2: cdev add\n");
            goto err_free_buf;
        }
    
    printk (KERN_ALERT "Init done\n"); // printer til kernel at alt lykkedes
    return 0;    // succes
    

    err_free_buf: // ved fejl ved cdev_add
        gpio_free(16); // frigør GPIO port         
        unregister_chrdev_region(devno,ANTAL_DEVICES);    // afregistrerer

    err_exit:
    return err;
}
// exit funktion kaldes ved rmmod af driveren
static void __exit sw2_driver_exit(void)
{
    printk(KERN_ALERT "SW2 exit\n"); // printer at exit starter
    
    cdev_del(&my_cdev); // fjerner my_cdev
    
    unregister_chrdev_region(devno, ANTAL_DEVICES); // afregistrerer enhed
    
    gpio_free(16); // sætter gpio porten fri
       
    printk(KERN_ALERT "Exit done\n"); // printer at exit er færdig

}


// funktion ved åbning af GPIO device
int mygpio_open(struct inode *inode, struct file *filep)
{
    int major, minor;
    major = MAJOR(inode->i_rdev);
    minor = MINOR(inode->i_rdev);
	// printer til kernel at GPIO device er åbnet og tilhørende major og minor numre 
    printk("Opening MyGpio Device [major], [minor]: %i, %i\n", major, minor); 
    return 0;
}


// funktion ved GPIO device frigørelse
 int mygpio_release(struct inode *inode, struct file *filep)
{
    int minor, major;

    major = MAJOR(inode->i_rdev);
    minor = MINOR(inode->i_rdev);
	// printer til kernel at Device er frigjort 
    printk("Closing/Releasing MyGpio Device [major], [minor]: %i, %i\n", major, minor);

    return 0;
}


	// funktion til at læse fra SW2
 ssize_t my_switch_read(struct file *filep, char __user *buf, 
            size_t count, loff_t *f_pos)
 {
	 // init af variabler
    int err; 
    char value[3];
    int read;
    
	// læser fra GPIO port 16
    read = gpio_get_value(16);
    
	
    int len = sprintf(value,"%d",read); // vi konverterer fra int til C
    ++len;
    
    len = len > count ? count : len;
    err = copy_to_user(buf, value, len); // kopierer fra kernel til user space
        if (err != 0)
            printk("error ved ctu"); // printer error til kernel ved fejl
            
    *f_pos += len;
    return len;    
 }
 
 // struct til vores fops
 struct file_operations my_fops = {
.read = my_switch_read, //
.open = mygpio_open,
.release = mygpio_release};
 
module_init(sw2_driver_init); // vi kalder sw2_driver_init ved module init
module_exit(sw2_driver_exit); // vi kalder sw2_driver_exit ved module exit 
