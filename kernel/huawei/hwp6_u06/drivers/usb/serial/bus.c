/*
 * USB Serial Converter Bus specific functions
 *
 * Copyright (C) 2002 Greg Kroah-Hartman (greg@kroah.com)
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License version
 *	2 as published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/usb/serial.h>

/*< DTS00176579033101 begin: modified for usb tty devices number change 2012-03-31 >*/
extern struct usb_serial_port *port_keeping_table[SERIAL_TTY_MINORS];
extern int serial_reopen(struct tty_struct *tty, struct usb_serial_port *port);
extern struct mutex sr_mutex;
/*< DTS00176579033101 begin: modified for usb tty devices number change 2012-03-31 >*/

static int usb_serial_device_match(struct device *dev,
						struct device_driver *drv)
{
	struct usb_serial_driver *driver;
	const struct usb_serial_port *port;

	/*
	 * drivers are already assigned to ports in serial_probe so it's
	 * a simple check here.
	 */
	port = to_usb_serial_port(dev);
	if (!port)
		return 0;

	driver = to_usb_serial_driver(drv);

	if (driver == port->serial->type)
		return 1;

	return 0;
}

static ssize_t show_port_number(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct usb_serial_port *port = to_usb_serial_port(dev);

	return sprintf(buf, "%d\n", port->number - port->serial->minor);
}

static DEVICE_ATTR(port_number, S_IRUGO, show_port_number, NULL);

static int usb_serial_device_probe(struct device *dev)
{
	struct usb_serial_driver *driver;
	struct usb_serial_port *port;
	int retval = 0;
	int minor;

	port = to_usb_serial_port(dev);
	if (!port) {
		retval = -ENODEV;
		goto exit;
	}
	if (port->dev_state != PORT_REGISTERING)
		goto exit;

	driver = port->serial->type;
	if (driver->port_probe) {
		retval = driver->port_probe(port);
		if (retval)
			goto exit;
	}

	retval = device_create_file(dev, &dev_attr_port_number);
	if (retval) {
		if (driver->port_remove)
			retval = driver->port_remove(port);
		goto exit;
	}

	minor = port->number;
/*< DTS00176579033101 begin: modified for usb tty devices number change 2012-03-31 >*/
	mutex_lock(&sr_mutex);
	if (!port_keeping_table[minor]) {
	tty_register_device(usb_serial_tty_driver, minor, dev);
		dev_info(&port->serial->dev->dev,
			 "%s converter now attached to ttyUSB%d\n",
			 driver->description, minor);
	} else {
		struct usb_serial_port * old_port = port_keeping_table[minor];
		struct tty_struct *old_tty = old_port->port.tty;
		int old_counter = 0;	// ref counter for the old tty port
		int counter = 0;		// ref counter for the new tty port
		
		/* bind the old tty keep to the new tty port */
		serial_reopen(old_tty, port);
		
		port->port.count = old_port->port.count;
		old_tty->driver_data = port;
		old_counter = atomic_read(&((old_port->serial->kref).refcount));
		counter = atomic_read(&((port->serial->kref).refcount));
		atomic_set(&((port->serial->kref).refcount),old_counter);
		atomic_set(&((old_port->serial->kref).refcount),counter);
		
		/* release the old serial and tty port */
		old_port->serial->minor = SERIAL_TTY_NO_MINOR;
		usb_serial_put(old_port->serial);
		port_keeping_table[minor] = NULL;
	}
	mutex_unlock(&sr_mutex);
/*< DTS00176579033101 end: modified for usb tty devices number change 2012-03-31 >*/

exit:
	return retval;
}

static int usb_serial_device_remove(struct device *dev)
{
	struct usb_serial_driver *driver;
	struct usb_serial_port *port;
	int retval = 0;
	int minor;

	port = to_usb_serial_port(dev);
	if (!port)
		return -ENODEV;

	if (port->dev_state != PORT_UNREGISTERING)
		return retval;

	device_remove_file(&port->dev, &dev_attr_port_number);

	driver = port->serial->type;
	if (driver->port_remove)
		retval = driver->port_remove(port);

	minor = port->number;
/*< DTS00176579033101 begin: modified for usb tty devices number change 2012-03-31 >*/
	mutex_lock(&sr_mutex);
	if (port->port.count == 0) {
		tty_unregister_device(usb_serial_tty_driver, minor);
		dev_info(dev, "%s converter now disconnected from ttyUSB%d\n",
			 driver->description, minor);
	} else {
		/* serial port in using, so put into keeping table */
		port_keeping_table[minor] = port;
	}
	mutex_unlock(&sr_mutex);
/*< DTS00176579033101 end: modified for usb tty devices number change 2012-03-31 >*/

	return retval;
}

#ifdef CONFIG_HOTPLUG
static ssize_t store_new_id(struct device_driver *driver,
			    const char *buf, size_t count)
{
	struct usb_serial_driver *usb_drv = to_usb_serial_driver(driver);
	ssize_t retval = usb_store_new_id(&usb_drv->dynids, driver, buf, count);

	if (retval >= 0 && usb_drv->usb_driver != NULL)
		retval = usb_store_new_id(&usb_drv->usb_driver->dynids,
					  &usb_drv->usb_driver->drvwrap.driver,
					  buf, count);
	return retval;
}

static struct driver_attribute drv_attrs[] = {
	__ATTR(new_id, S_IWUSR, NULL, store_new_id),
	__ATTR_NULL,
};

static void free_dynids(struct usb_serial_driver *drv)
{
	struct usb_dynid *dynid, *n;

	spin_lock(&drv->dynids.lock);
	list_for_each_entry_safe(dynid, n, &drv->dynids.list, node) {
		list_del(&dynid->node);
		kfree(dynid);
	}
	spin_unlock(&drv->dynids.lock);
}

#else
static struct driver_attribute drv_attrs[] = {
	__ATTR_NULL,
};
static inline void free_dynids(struct usb_serial_driver *drv)
{
}
#endif

struct bus_type usb_serial_bus_type = {
	.name =		"usb-serial",
	.match =	usb_serial_device_match,
	.probe =	usb_serial_device_probe,
	.remove =	usb_serial_device_remove,
	.drv_attrs = 	drv_attrs,
};

int usb_serial_bus_register(struct usb_serial_driver *driver)
{
	int retval;

	driver->driver.bus = &usb_serial_bus_type;
	spin_lock_init(&driver->dynids.lock);
	INIT_LIST_HEAD(&driver->dynids.list);

	retval = driver_register(&driver->driver);

	return retval;
}

void usb_serial_bus_deregister(struct usb_serial_driver *driver)
{
	free_dynids(driver);
	driver_unregister(&driver->driver);
}

