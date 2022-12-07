/* Necessary includes for device drivers */
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

/* Declaration of prodcons.c functions */
int prodcons_open(struct inode *inode, struct file *filp);
int prodcons_release(struct inode *inode, struct file *filp);
ssize_t prodcons_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
ssize_t prodcons_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
void prodcons_exit(void);
int prodcons_init(void);

/* Structure that declares the usual file */
/* access functions */
struct file_operations prodcons_fops = {
  read: prodcons_read,
  write: prodcons_write,
  open: prodcons_open,
  release: prodcons_release
};

/* Declaration of the init and exit functions */
module_init(prodcons_init);
module_exit(prodcons_exit);

/*** El driver para lecturas sincronas *************************************/

#define TRUE 1
#define FALSE 0

/* Global variables of the driver */

int prodcons_major = 61;     /* Major number */

/* Buffer to store data */
#define MAX_SIZE 8192

static char *prodcons_buffer;
static ssize_t curr_size;
static int readers;
static int writing;
static int pend_open_write;
static int gcount=0;

/* El mutex y la condicion para prodcons */
static KMutex mutex;
static KCondition cond;

int prodcons_init(void) {
  int rc;

  /* Registering device */
  rc = register_chrdev(prodcons_major, "prodcons", &prodcons_fops);
  if (rc < 0) {
    printk(
      "<1>prodcons: cannot obtain major number %d\n", prodcons_major);
    return rc;
  }

  readers= 0;
  writing= FALSE;
  pend_open_write= 0;
  curr_size= 0;
  m_init(&mutex);
  c_init(&cond);

  /* Allocating prodcons_buffer */
  prodcons_buffer = kmalloc(MAX_SIZE, GFP_KERNEL);
  if (prodcons_buffer==NULL) {
    prodcons_exit();
    return -ENOMEM;
  }
  memset(prodcons_buffer, 0, MAX_SIZE);

  printk("<1>prodconsInserting prodcons module\n");
  return 0;
}

void prodcons_exit(void) {
  /* Freeing the major number */
  unregister_chrdev(prodcons_major, "prodcons");

  /* Freeing buffer prodcons */
  if (prodcons_buffer) {
    kfree(prodcons_buffer);
  }

  printk("<1>prodconsRemoving prodcons module\n");
}

int prodcons_open(struct inode *inode, struct file *filp) {
  int rc= 0;
  m_lock(&mutex);

  if (filp->f_mode & FMODE_WRITE) {
    int rc;
    printk("<1>prodconsopen request for write\n");
    /* Se debe esperar hasta que no hayan otros lectores o escritores */
    pend_open_write++;
    while (writing || readers>0) {
      if (c_wait(&cond, &mutex)) {
        pend_open_write--;
        c_broadcast(&cond);
        rc= -EINTR;
        goto epilog;
      }
    }
    writing= TRUE;
    pend_open_write--;
    curr_size= 0;
    c_broadcast(&cond);
    printk("<1>prodconsopen for write successful\n");
  }
  else if (filp->f_mode & FMODE_READ) {
    /* Para evitar la hambruna de los escritores, si nadie esta escribiendo
     * pero hay escritores pendientes (esperan porque readers>0), los
     * nuevos lectores deben esperar hasta que todos los lectores cierren
     * el dispositivo e ingrese un nuevo escritor.
     */
    while (!writing && pend_open_write>0) {
      if (c_wait(&cond, &mutex)) {
        rc= -EINTR;
        goto epilog;
      }
    }
    readers++;
    printk("<1>prodconsopen for read\n");
  }

epilog:
  m_unlock(&mutex);
  return rc;
}

int prodcons_release(struct inode *inode, struct file *filp) {
  m_lock(&mutex);

  if (filp->f_mode & FMODE_WRITE) {
    writing= FALSE;
    c_broadcast(&cond);
    printk("<1>prodconsclose for write successful\n");
  }
  else if (filp->f_mode & FMODE_READ) {
    readers--;
    if (readers==0)
      c_broadcast(&cond);
    printk("<1>prodconsclose for read (readers remaining=%d)\n", readers);
  }

  m_unlock(&mutex);
  return 0;
}

ssize_t prodcons_read(struct file *filp, char *buf,
                    size_t count, loff_t *f_pos) {
  ssize_t rc;
  m_lock(&mutex);

  while (curr_size <= *f_pos && writing) {
    /* si el lector esta en el final del archivo pero hay un proceso
     * escribiendo todavia en el archivo, el lector espera.
     */
    if (c_wait(&cond, &mutex)) {
      printk("<1>prodconsread interrupted gcount=%d\n", gcount);
      rc= -EINTR;
      goto epilog;
    }
  }

  if (count > curr_size-*f_pos) {
    // printk("<1>prodconsread BAD SIZE 2 READ");
    count= curr_size-*f_pos;
  }

  printk("<1>prodconsread read gcount=%d\n", gcount);

  /* Transfiriendo datos hacia el espacio del usuario */
  if (copy_to_user(buf, prodcons_buffer+0, gcount)!=0) {
    /* el valor de buf es una direccion invalida */
    printk("<1>prodconsread BAD READ");
    rc= -EFAULT;
    goto epilog;
  }

  *f_pos+= count;
  rc= count;
  gcount=0;

epilog:
  m_unlock(&mutex);
  return rc;
}

ssize_t prodcons_write( struct file *filp, const char *buf,
                      size_t count, loff_t *f_pos) {
  ssize_t rc;
  loff_t last;

  m_lock(&mutex);

  last= *f_pos + count;
  if (last>MAX_SIZE) {
    count -= last-MAX_SIZE;
  }

  /* Transfiriendo datos desde el espacio del usuario */
  if (copy_from_user(prodcons_buffer+0, buf, count)!=0) {
    /* el valor de buf es una direccion invalida */
    printk("<1>prodconsread BAD WRITE");
    rc= -EFAULT;
    goto epilog;
  }

  *f_pos += count;
  curr_size= *f_pos;
  rc= count;
  gcount=count;
  printk("<1>prodcons write gcount=%d", gcount);
  c_signal(&cond);

epilog:
  m_unlock(&mutex);
  return rc;
}

