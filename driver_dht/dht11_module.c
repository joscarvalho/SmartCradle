#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <asm/io.h>
#include <linux/timer.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include "../inc/gpio.h"
#include "../inc/dht.h"

#define DEVICE_NAME "dht110"
#define CLASS_NAME "dht11Class"

/* Device variables */
static struct class* dht11Device_class = NULL;
static struct device* dht11Device_device = NULL;
static dev_t dht11Device_majorminor;
static struct cdev c_dev;  // Character device structure

/* Registers */
struct GpioRegisters *pGPIORegisters;

/* Device Pin */
static const int DATA = 25;

/*
 * Waits for the GPIO to change its level before reaching
 * the timeout (in us).
 */ 
static int wait(struct GpioRegisters *pGPIORegisters, int GPIO, bool level, int timeout)
{
    int cnt = 0;
    while (level == GetGPIOValue(pGPIORegisters,GPIO)) {
        if (cnt > timeout) {
            return 0;
        }
        cnt++;
        udelay(1);
    }
    return 1;
}

ssize_t dht11_device_read(struct file *pfile, char __user *p_buff,size_t len, loff_t *poffset){
	struct GpioRegisters *pdev; 

	bool lastState = 0;
	uint8_t bitCounter = 0;
	uint8_t timeCounter = 0;

	dht11_t sensorData;

	pr_alert("%s: called\n",__FUNCTION__);
	
	if(unlikely(pfile->private_data == NULL))
		return -EFAULT;

	pdev = (struct GpioRegisters *)pfile->private_data;

	SetGPIOFunction(pdev, DATA, OUTPUT); //Set pin to output
	SetGPIOValue(pdev,DATA,LOW); //Set pin LOW
	mdelay(18); //Wait for 18ms
	SetGPIOValue(pdev,DATA,HIGH); //Set pin HIGH
	udelay(40); //Wait for 40us
	SetGPIOFunction(pdev, DATA, INPUT); //Set pin to input

	if(!wait(pdev, DATA, 0,100)) //Wait for the sensor's acknowledge
	{
		pr_alert("%s: Timeout. No ACK from sensor.\n",__FUNCTION__,len);
		return 0;
	}

	if(!wait(pdev, DATA, 1,100)) //Wait for the sensor's response
	{
		pr_alert("%s: Timeout.\n",__FUNCTION__,len);
		return 0;
	}

	int i;
	for(i = 0; i < 80; i++)
	{
		timeCounter=0;
		
		while(GetGPIOValue(pdev,DATA) == lastState)
		{
			timeCounter++;
			udelay(1);
			if(timeCounter == 255)
			{
				pr_alert("%s: Sampling error.\n",__FUNCTION__,len);
				return 0;
			}
		}
		lastState = GetGPIOValue(pdev,DATA);

		if( ( i >= 1) && (i % 2) ) //Only get data bits
		{
			sensorData.CompleteSample[bitCounter / 8] <<= 1;
			if(timeCounter > 30)
				sensorData.CompleteSample[bitCounter / 8] |= 1;
			bitCounter++;
		}
	}

	if( bitCounter >= 40)
	{
		//Calculate the checksum
		u_int8_t checksum=0;
		for(i = 0; i < 4; i++)
			checksum += sensorData.CompleteSample[i];
		sensorData.checksum=checksum;

		if(sensorData.checksum != sensorData.CompleteSample[4]) //Verify the checksum
		{
			pr_alert("%s: Sampling error. Checksum mismatch: %d != %d\n",__FUNCTION__,sensorData.checksum,sensorData.CompleteSample[4]);
			return 0;
		}

		sensorData.HumidityI = sensorData.CompleteSample[0];
		sensorData.HumidityD = sensorData.CompleteSample[1];
		sensorData.TemperatureI = sensorData.CompleteSample[2];
		sensorData.TemperatureD = sensorData.CompleteSample[3];

		int ret;
		do{
			ret=copy_to_user(p_buff,(void *)&sensorData,sizeof(sensorData)); //Send to user space
		}while(ret);

		return len;
	}

	return 0;
}

int dht11_device_close(struct inode *p_inode, struct file * pfile){
	
	pr_alert("%s: called\n",__FUNCTION__);
	pfile->private_data = NULL;
	return 0;
}

int dht11_device_open(struct inode* p_indode, struct file *p_file){

	pr_alert("%s: called\n",__FUNCTION__);
	p_file->private_data = (struct GpioRegisters *) pGPIORegisters;
	return 0;
}

static struct file_operations dht11Device_fops = {
	.owner = THIS_MODULE,
	.read = dht11_device_read,
	.release = dht11_device_close,
	.open = dht11_device_open,
};

static int __init dht11Module_init(void) {
	int ret;
	struct device *dev_ret;

	pr_alert("%s: called\n",__FUNCTION__);

	if ((ret = alloc_chrdev_region(&dht11Device_majorminor, 0, 1, DEVICE_NAME)) < 0) {
		return ret;
	}

	if (IS_ERR(dht11Device_class = class_create(THIS_MODULE, CLASS_NAME))) {
		unregister_chrdev_region(dht11Device_majorminor, 1);
		return PTR_ERR(dht11Device_class);
	}
	if (IS_ERR(dev_ret = device_create(dht11Device_class, NULL, dht11Device_majorminor, NULL, DEVICE_NAME))) {
		class_destroy(dht11Device_class);
		unregister_chrdev_region(dht11Device_majorminor, 1);
		return PTR_ERR(dev_ret);
	}

	cdev_init(&c_dev, &dht11Device_fops);
	c_dev.owner = THIS_MODULE;
	if ((ret = cdev_add(&c_dev, dht11Device_majorminor, 1)) < 0) {
		printk(KERN_NOTICE "Error %d adding device", ret);
		device_destroy(dht11Device_class, dht11Device_majorminor);
		class_destroy(dht11Device_class);
		unregister_chrdev_region(dht11Device_majorminor, 1);
		return ret;
	}

	pGPIORegisters = (struct GpioRegisters *)ioremap(GPIO_BASE, sizeof(struct GpioRegisters));

	SetGPIOFunction(pGPIORegisters, DATA, INPUT); //Input
	SetGPIOPull(pGPIORegisters,DATA,NO_PULL);

	pr_alert("map to virtual adresse: 0x%x\n", (unsigned)pGPIORegisters);

	return 0;
}

static void __exit dht11Module_exit(void) {
	
	pr_alert("%s: called\n",__FUNCTION__);

	iounmap(pGPIORegisters);
	cdev_del(&c_dev);
	device_destroy(dht11Device_class, dht11Device_majorminor);
	class_destroy(dht11Device_class);
	unregister_chrdev_region(dht11Device_majorminor, 1);
}

module_init(dht11Module_init);
module_exit(dht11Module_exit);

MODULE_LICENSE("GPL");