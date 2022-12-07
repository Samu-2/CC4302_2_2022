/*
 
         __   __        __                  __   __   __  ___ 
 |    | |__) |__)  /\  |__) \ /    |  |\/| |__) /  \ |__)  |  
 |___ | |__) |  \ /~~\ |  \  |     |  |  | |    \__/ |  \  |  
                                                              
 
*/
#include <linux/init.h>
/* #include <linux/config.h> */
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#include <linux/uaccess.h> /* copy_from/to_user */
#include "kmutex.h"
MODULE_LICENSE("Dual BSD/GPL");
#define TRUE 1
#define FALSE 0
#define log(MSG) printk("<1>prodcons: " MSG);

/*
 
  ___            __  ___    __           __   ___  __             __       ___    __       
 |__  |  | |\ | /  `  |  | /  \ |\ |    |  \ |__  /  ` |     /\  |__)  /\   |  | /  \ |\ | 
 |    \__/ | \| \__,  |  | \__/ | \|    |__/ |___ \__, |___ /~~\ |  \ /~~\  |  | \__/ | \| 
                                                                                           
 
*/
static int      prodcons_open(struct inode *inode, struct file *filp);
static int      prodcons_release(struct inode *inode, struct file *filp);
static ssize_t  prodcons_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
static ssize_t  prodcons_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
int prodcons_init(void); 
void prodcons_exit(void);
module_exit(prodcons_exit);
module_init(prodcons_init);

/* access functions */
struct file_operations prodcons_fops = {
  read: prodcons_read,
  write: prodcons_write,
  open: prodcons_open,
  release: prodcons_release
};

/*
 
  ___            __  ___    __        __              __        ___        ___      ___      ___    __        __  
 |__  |  | |\ | /  `  |  | /  \ |\ | /__`    |  |\/| |__) |    |__   |\/| |__  |\ |  |   /\   |  | /  \ |\ | /__` 
 |    \__/ | \| \__,  |  | \__/ | \| .__/    |  |  | |    |___ |___  |  | |___ | \|  |  /~~\  |  | \__/ | \| .__/ 
                                                                                                                  
 
*/
#define MAX_BUFFER_SIZE 8192
int prodcons_major = 61;
static KCondition   cond;
static KMutex       mutex;
static char*        prodconBuf;

int prodcons_init(void) {
    log ("loading module\n");
    // declarar y pedir memoria
    prodconBuf = kmalloc(MAX_BUFFER_SIZE, GFP_KERNEL);
    if (prodconBuf == NULL) {
        prodcons_exit();
        return -ENOMEM;
    }
    m_init(&mutex);
    c_init(&cond);
    memset(prodconBuf, 0, MAX_BUFFER_SIZE);
    int returnCode = register_chrdev(prodcons_major, "prodcons", &prodcons_fops);
    if (returnCode != 0) {
        printk("<1>prodcons: Cannot obtain major number %d\n", prodcons_major);
        return returnCode;
    }
    log ("Loaded module\n");
    return 0;
}

void prodcons_exit(void) {
    printk("<1>prodcons: Removing module\n");
    unregister_chrdev(prodcons_major, "prodcons");
    // TODO: Free memory used
    if (prodconBuf) {
        kfree(prodconBuf);
    }
    printk("<1>prodcons: Removed module\n");
}

static int prodcons_open(struct inode *inode, struct file *filp) {
    if (filp->f_mode & FMODE_WRITE) {   
        // TODO something with writers
        printk("<1>prodcons: ENTRA ESCRITOR");
    }
    else if (filp->f_mode & FMODE_READ) {
        // TODO something with readers
        printk("<1>prodcons: ENTRA LECTOR");
    } 
    printk("<1>prodcons: released open\n");
    return 0;
    // un proceso abre el dispositivo 
    char *mode=   filp->f_mode & FMODE_WRITE ? "write" :
                  filp->f_mode & FMODE_READ ? "read" :
                  "unknown";
    printk("<1>prodcons: open %p for %s\n", filp, mode);
    return 0;
}

static int prodcons_release(struct inode *inode, struct file *filp) {
    // Si se pidieron recursos en open se liberan
    if (filp->f_mode & FMODE_WRITE) {
        // TODO something with writers
    }
    else if (filp->f_mode & FMODE_READ) {
        // TODO something with readers
    } 
    printk("<1>prodcons: released\n");
    return 0;
}

static ssize_t prodcons_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
    // logica de lectura
    // filp:    file
    // buf:     buffer de salida
    // count:   chars a leer
    // f_pos:   posicion en la que leer
    ssize_t returnCode = 0;
    if (c_wait(&cond, &mutex)) {
        printk("<1>prodcons: Read interrupted");
        returnCode = -EINTR;
        return returnCode;
    }
    // if (count > curr_size-*f_pos)
    //     count= curr_size-*f_pos;
    if (copy_to_user(buf, prodconBuf, count)!=0) {
        /* el valor de buf es una direccion invalida */
        printk("<1>prodcons: Bad read");
        returnCode= -EFAULT;
        return returnCode;
    }
    returnCode = count;
    m_unlock(&mutex);
    return returnCode;
}

static ssize_t prodcons_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
    // logica de escritura
    ssize_t returnCode;
    m_lock(&mutex);
    if (!copy_from_user(prodconBuf, buf, count)){
        returnCode = -EFAULT;
        goto exit;
    }
    returnCode = count;
    c_signal(&cond);
exit:
    m_unlock(&mutex);
    return returnCode;
}