
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/mtd/nve_interface.h>

#define     UART_OUTPUT_NAME            "UARTOUT"
#define     UART_OUTPUT_NUM             (305)
#define     UART_OUTPUT_ENABLE          1
#define     UART_OUTPUT_DISABLE         0

bool if_uart_output_enabled(void)
{
	struct nve_info_user pinfo;
	int ret;

	pinfo.nv_operation = NV_READ;
	pinfo.nv_number = UART_OUTPUT_NUM;
	pinfo.valid_size = 1;
	strncpy(pinfo.nv_name, UART_OUTPUT_NAME, NV_NAME_LENGTH);

	ret = nve_direct_access(&pinfo);

	if (-ENODEV == ret)
	{
		//mtd device is not ready or nve doesn't exist
		if (NULL != strstr(saved_command_line, "console=ttyAMA0,115200"))
			return true;
	}

	if ((0 == ret) && (1 == pinfo.nv_data[0]))
	{
		return true;
	}

	return false;
}

static ssize_t uart_output_read(struct file *filp,  char  __user *buffer, size_t count, loff_t *ppos)
{
	int len = 0;
	char state[8];
	memset(state, 0, 8);

	len = sprintf(state, "%d\n", if_uart_output_enabled()? 1 : 0);

	return simple_read_from_buffer(buffer, count, ppos, (void *)state, len);
}

static ssize_t uart_output_write(struct file *filp,  const char __user *buf, size_t cnt, loff_t *ppos)
{
	char *control;
	struct nve_info_user pinfo;

	pinfo.nv_operation = NV_WRITE;
	pinfo.nv_number = UART_OUTPUT_NUM;
	pinfo.valid_size = 1;
	strncpy(pinfo.nv_name, UART_OUTPUT_NAME, NV_NAME_LENGTH);
	control = kmalloc(cnt, GFP_KERNEL);
	if (control == NULL)
		return 0;

	if (copy_from_user(control, buf, cnt)) {
		pr_err("Copy from user failed\n");
		kfree(control);
		return 0;
	}

	if (control[0] == '1')
	{
		pinfo.nv_data[0] = 1;
		nve_direct_access(&pinfo);
		// enable uart if possible now
	}
	else if (control[0] == '0')
	{
		pinfo.nv_data[0] = 0;
		nve_direct_access(&pinfo);
		// disable uart if possible now
	}
	else
	{
		kfree(control);
		return (ssize_t)cnt;
	}

	kfree(control);
	return (ssize_t)cnt;
}

static const struct file_operations uart_output_fops = {
		.read = uart_output_read,
		.write = uart_output_write,
};

int  uart_output_debugfs_init(void)
{
	struct dentry *uart_output_root, *state_file;

	uart_output_root = debugfs_create_dir("uart_output", NULL);
	if (!uart_output_root)
		return -ENOENT;

	state_file = debugfs_create_file("state", 0644, uart_output_root, NULL, &uart_output_fops);

	return 0;
}
