
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <hsad/config_general_struct.h>

#include <hsad/config_mgr.h>
#include <hsad/config_interface.h>

#include <mach/boardid.h>
#include <hsad/config_debugfs.h>


#define BOARDID_CONFIG_FILE_SIZE (PAGE_SIZE * 2)

struct dentry *boardid_debugfs_root;
struct dentry *boardid_common_root;
static ssize_t config_boardid_read(struct file *filp,  char  __user *buffer, size_t count, loff_t *ppos)
{
	int len = 0;
	char idarray[32];
	memset(idarray, 0, 32);

	len = sprintf(idarray, "0x%02x\n", get_boardid());
	return simple_read_from_buffer(buffer, count, ppos, (void *)idarray, len);
}

static ssize_t config_chipid_read(struct file *filp,  char  __user *buffer, size_t count, loff_t *ppos)
{
	int len = 0;
	char idarray[32];
	memset(idarray, 0, 32);

	len = sprintf(idarray, "0x%08x\n", get_chipid());
	return simple_read_from_buffer(buffer, count, ppos, (void *)idarray, len);
}
static ssize_t config_pmuid_read(struct file *filp,  char  __user *buffer, size_t count, loff_t *ppos)
{
	int len = 0;
	char idarray[32];
	memset(idarray, 0, 32);

	len = sprintf(idarray, "0x%02x\n", get_pmuid());
	return simple_read_from_buffer(buffer, count, ppos, (void *)idarray, len);
}

#define DEBUGFS_ROOT_PATH "sys/kernel/debug/boardid/common"
#define DEBUGFS_PATH_MAX_LENGTH 128

struct dir_node {
	struct dentry *dentry;
	struct list_head list;
	char  path[DEBUGFS_PATH_MAX_LENGTH];
};

/* A list used for store dentry struct while creating the debugfs tree */
static struct list_head g_dentry_list;

static ssize_t config_common_read(struct file *filp, char __user *buffer, size_t count, loff_t *ppos);

static const struct file_operations config_common_fops = {
	.read = config_common_read,
};

/*
 * Description: try to find a dentry from g_dentry_list, use the path as a match key.
 * Return Value: 0 for failure, and return a pointer to a dentry structure if success.
 * */
static struct dentry *find_dentry_from_path(char *path)
{
	struct list_head *pos;
	struct dir_node *p;

	list_for_each(pos, &g_dentry_list)
	{
		p = container_of(pos, struct dir_node, list);

		if (!strcmp(path, p->path)) {
			return p->dentry;
		}
	}

	return NULL;
}

/*
 * Description: Allocate a dir node for pdentry and add it to g_dentry_list, with
 *              the path as a find-use key.
 * */
static int add_dentry_to_list(struct dentry *pdentry, char *path)
{
	struct dir_node *pNode = kmalloc(sizeof(struct dir_node), GFP_KERNEL);

	if (pNode) {
		pNode->dentry = pdentry;
		strcpy(pNode->path, path);
		list_add(&(pNode->list), &g_dentry_list);
		return 0;
	} else
		return -ENOMEM;
}
/*
 * Description: This function destroys the g_dentry_list, including
 *              including free the memory allocated before.
 * */
static int destroy_dentry_list(void)
{
	struct list_head *pos, *next;
	struct dir_node *p;
	list_for_each_safe(pos, next, &g_dentry_list)
	{
		p = container_of(pos, struct dir_node, list);
		list_del(pos);
		kfree(p);
	}
	return 0;
}

/* This function create debugfs for  a config key */
static int generate_common_debugfs_line(struct dentry *root, const char * key)
{
	char *buf, *fullpath, *b;
	struct dentry *parent = root;
	int ret = 0;

	buf = kmalloc(sizeof(char)*DEBUGFS_PATH_MAX_LENGTH, GFP_KERNEL);
	if (!buf) {
		ret = -ENOMEM;
		goto fail;
	}

	fullpath = kmalloc(sizeof(char)*DEBUGFS_PATH_MAX_LENGTH, GFP_KERNEL);
	if (!fullpath) {
		ret = -ENOMEM;
		goto fail;
	}

	/* copy buffer for later use */
	strlcpy(buf, key, sizeof(char)*DEBUGFS_PATH_MAX_LENGTH);

	/* copy buff for fullpath, which contains a file's the absolute path*/
	strlcpy(fullpath, DEBUGFS_ROOT_PATH, sizeof(char) * DEBUGFS_PATH_MAX_LENGTH);

	b = strim(buf);

	while (b) {
		char *name = strsep(&b, "/");
		if (!name)
			continue;
		if (b) {
			/* the name represents a directory. */
			struct dentry *pdentry;
			strcat(fullpath, "/");
			strcat(fullpath, name);
			pdentry = find_dentry_from_path(fullpath);
			if (!pdentry) {
				/* the directory is not created before. */
				parent = debugfs_create_dir(name, parent);
				add_dentry_to_list(parent, fullpath);
			} else {
				/* the directory is created before, just parse next name */
				parent = pdentry;
			}
		} else {
			/* the name represents a file */
			debugfs_create_file(name, 0444, parent, NULL, &config_common_fops);
		}
	}

fail:
	if (buf)
		kfree(buf);
	if (fullpath)
		kfree(fullpath);
	return ret;
}

static int generate_common_debugfs_tree(struct dentry *root)
{
	struct board_id_general_struct *config_pairs_ptr;
	config_pair *pconfig;

	config_pairs_ptr = get_board_id_general_struct(COMMON_MODULE_NAME);

	if (NULL == config_pairs_ptr) {
		return -ENOENT;
	}

	INIT_LIST_HEAD(&g_dentry_list);

	pconfig = (config_pair *)config_pairs_ptr->data_array.config_pair_ptr;
	while (NULL != pconfig->key) {
		/* create every key in debugfs now. */
		int ret = generate_common_debugfs_line(root, pconfig->key);
		if (0 != ret)
			return ret;
		pconfig++;
	}

	return 0;
}

static ssize_t config_common_file_read(struct file *filp, char __user *buffer, size_t count, loff_t *ppos)
{
	struct board_id_general_struct *config_pairs_ptr;
	config_pair *pconfig ;
	unsigned int len = 0, cnt = 0;
	char *ptr, *arr_ptr_tmp;
	config_pairs_ptr = get_board_id_general_struct(COMMON_MODULE_NAME);

	ptr = (char *)kmalloc((BOARDID_CONFIG_FILE_SIZE*sizeof(char)), GFP_KERNEL);

	if (!ptr)
		return -ENOMEM;

	memset(ptr, 0, BOARDID_CONFIG_FILE_SIZE);

	arr_ptr_tmp = ptr;

	if (NULL == config_pairs_ptr) {
		HW_CONFIG_DEBUG(" can not find  module:common\n");
		len = scnprintf(ptr, BOARDID_CONFIG_FILE_SIZE, " can not find  module:common\n");
		len = simple_read_from_buffer(buffer, count, ppos, (void *)ptr, len);
		kfree(ptr);
		return len;
	}

	pconfig = (config_pair *)config_pairs_ptr->data_array.config_pair_ptr;
	len = scnprintf(ptr, BOARDID_CONFIG_FILE_SIZE, "%-20s\t%-20s\t%-10s\n\n", "COMMON KEY", "COMMON DATA", "COMMON TYPE");
	ptr += len;
	cnt += len;
	while (NULL != pconfig->key) {
		if (BOARDID_CONFIG_FILE_SIZE <= cnt + 1) {
			HW_CONFIG_DEBUG("max size over one page");
			break;
		}

		if (0 == strlen(pconfig->key)) {
			pconfig++;
			continue;
		}

		if (E_CONFIG_DATA_TYPE_STRING == pconfig->type) {
			len = scnprintf(ptr, BOARDID_CONFIG_FILE_SIZE-cnt, "%-20s\t%-20s\t%-3d(string)\n", pconfig->key, (char *)pconfig->data, pconfig->type);
		} else if (E_CONFIG_DATA_TYPE_INT == pconfig->type) {
			len = scnprintf(ptr, BOARDID_CONFIG_FILE_SIZE-cnt, "%-20s\t%-20d\t%-3d(int)\n", pconfig->key, pconfig->data, pconfig->type);
		} else if (E_CONFIG_DATA_TYPE_BOOL == pconfig->type) {
			len = scnprintf(ptr, BOARDID_CONFIG_FILE_SIZE-cnt, "%-20s\t%-20d\t%-3d(bool)\n", pconfig->key, pconfig->data, pconfig->type);
		} else if (E_CONFIG_DATA_TYPE_ENUM == pconfig->type) {
			len = scnprintf(ptr, BOARDID_CONFIG_FILE_SIZE-cnt, "%-20s\t%-20d\t%-3d(enum)\n", pconfig->key, pconfig->data, pconfig->type);
		} else {
			len = scnprintf(ptr, BOARDID_CONFIG_FILE_SIZE-cnt, "%-20s\t%-20d\t%-3d(unknow type)\n", pconfig->key, pconfig->data, pconfig->type);
		}

		ptr += len;
		cnt += len;
		pconfig++;
	}

	len = strlen(arr_ptr_tmp);

	len = simple_read_from_buffer(buffer, count, ppos, (void *) arr_ptr_tmp, len);
	kfree(arr_ptr_tmp);
	return len;

}

static ssize_t config_common_read(struct file *filp, char __user *buffer, size_t count, loff_t *ppos)
{
	struct board_id_general_struct *config_pairs_ptr;
	config_pair *pconfig;
	unsigned int len = 0;
	unsigned int ret = 0;
	char *ptr      = NULL;
	char *fullpath = NULL;
	char *pkey     = NULL;
	char *ppath    = NULL;

	config_pairs_ptr = get_board_id_general_struct(COMMON_MODULE_NAME);

	fullpath = (char *)kmalloc((DEBUGFS_PATH_MAX_LENGTH*sizeof(char)), GFP_KERNEL);
	if (!fullpath) {
		ret = -ENOMEM;
		goto fail;
	}

	ptr = (char *)kmalloc((PAGE_SIZE*sizeof(char)), GFP_KERNEL);
	if (!ptr) {
		ret = -ENOMEM;
		goto fail;
	}

	memset(ptr, 0, PAGE_SIZE);
	memset(fullpath, 0, sizeof(char)*DEBUGFS_PATH_MAX_LENGTH);

	ppath = d_path((const struct path *)&filp->f_path, fullpath, sizeof(char)*DEBUGFS_PATH_MAX_LENGTH);

	if (NULL == config_pairs_ptr) {
		HW_CONFIG_DEBUG(" can not find  module:common\n");
		len = scnprintf(ptr, PAGE_SIZE, " can not find  module:common\n");
		len = simple_read_from_buffer(buffer, count, ppos, (void *)ptr, len);
		kfree(ptr);
		return len;
	}

	pkey = strstr(ppath, DEBUGFS_ROOT_PATH"/");
	if (pkey) {
		pkey += strlen(DEBUGFS_ROOT_PATH"/");
	}

	pconfig = (config_pair *)config_pairs_ptr->data_array.config_pair_ptr;
	while ((NULL != pconfig->key) && pkey) {
		if (!strcmp(pkey, pconfig->key)) {
			if (E_CONFIG_DATA_TYPE_STRING == pconfig->type) {
				len = scnprintf(ptr, PAGE_SIZE, "string:%s\n", (char *)pconfig->data);
			} else if (E_CONFIG_DATA_TYPE_INT == pconfig->type) {
				len = scnprintf(ptr, PAGE_SIZE, "int:%d\n", pconfig->data);
			} else if (E_CONFIG_DATA_TYPE_BOOL == pconfig->type) {
				len = scnprintf(ptr, PAGE_SIZE, "bool:%s\n", pconfig->data ? "yes" : "no");
			} else if (E_CONFIG_DATA_TYPE_ENUM == pconfig->type) {
				len = scnprintf(ptr, PAGE_SIZE, "enum:%d\n", pconfig->data);
			} else {
				len = scnprintf(ptr, PAGE_SIZE, "unknow type:%d\n", pconfig->data);
			}
			break;
		}
		pconfig++;
	}

	ret = simple_read_from_buffer(buffer, count, ppos, (void *)ptr, len);

fail:
	if (fullpath) {
		kfree(fullpath);
	}
	if (ptr) {
		kfree(ptr);
	}
	return ret;
}


#ifdef CONFIG_HUAWEI_GPIO_UNITE
static ssize_t config_gpio_read(struct file *filp,  char  __user *buffer, size_t count, loff_t *ppos)
{
	struct board_id_general_struct *gpios_ptr = get_board_id_general_struct(GPIO_MODULE_NAME);
	gpiomux_setting *gpio_ptr;
	char *ptr, *arr_ptr_tmp;
	int len = 0, cnt = 0;
	ptr = (char *)kmalloc((PAGE_SIZE*sizeof(char)), GFP_KERNEL);

	if (!ptr)
		return -ENOMEM;

	memset(ptr, 0, PAGE_SIZE);

	arr_ptr_tmp = ptr;

	if (NULL == gpios_ptr) {
		HW_CONFIG_DEBUG(" can not find  module:gpio\n");
		len = scnprintf(ptr, PAGE_SIZE, " can not find  module:gpio\n");
		len = simple_read_from_buffer(buffer, count, ppos, (void *)ptr, len);
		kfree(ptr);
		return len;
	}

	gpio_ptr = (gpiomux_setting *) gpios_ptr->data_array.gpio_ptr;
	len = scnprintf(ptr, PAGE_SIZE, "%-20s\t%-20s\t%-10s\n", "NET NAME", "GENERAL NAME", "GPIO NUMBER");
	ptr += len;
	cnt += len;
	while (MUX_PIN_END != gpio_ptr->mux_pin) {
		if (PAGE_SIZE <= cnt + 1) {
			HW_CONFIG_DEBUG("max size over one page");
			break;
		}

		if (0 == strlen(gpio_ptr->net_name)) {
			gpio_ptr++;
			continue;
		}
		len = scnprintf(ptr, PAGE_SIZE-cnt, "%-20s\t%-20s\t%-3d\n", gpio_ptr->net_name, gpio_ptr->general_name, gpio_ptr->gpio_num);

		ptr += len;
		cnt += len;
		gpio_ptr++;
	}


	len = strlen(arr_ptr_tmp);
	len = simple_read_from_buffer(buffer, count, ppos, (void *)arr_ptr_tmp, len);
	kfree(arr_ptr_tmp);
	return len;

}
#endif

static const struct file_operations config_boardid_fops = {
		.read = config_boardid_read,
};

static const struct file_operations config_chipid_fops = {
		.read = config_chipid_read,
};
static const struct file_operations config_pmuid_fops = {
		.read = config_pmuid_read,
};

static const struct file_operations config_common_file_fops = {
		.read = config_common_file_read,
};

#ifdef CONFIG_HUAWEI_GPIO_UNITE
static const struct file_operations config_gpio_fops = {
		.read = config_gpio_read,
};
#endif

int  config_debugfs_init(void)
{
	struct dentry *common_file, *boardid_file, *chipid_file, *pmuid_file;

#ifdef CONFIG_HUAWEI_GPIO_UNITE
	struct dentry *gpio_file;
#endif

	boardid_debugfs_root = debugfs_create_dir("boardid", NULL);
	if (!boardid_debugfs_root)
		return -ENOENT;

	boardid_file = debugfs_create_file("boardid", 0444, boardid_debugfs_root, NULL, &config_boardid_fops);

	chipid_file = debugfs_create_file("chipid", 0444, boardid_debugfs_root, NULL, &config_chipid_fops);

	pmuid_file = debugfs_create_file("pmuid", 0444, boardid_debugfs_root, NULL, &config_pmuid_fops);

	boardid_common_root = debugfs_create_dir("common", boardid_debugfs_root);
	if (!boardid_common_root)
		return -ENOENT;

	common_file = debugfs_create_file("common", 0444, boardid_common_root, NULL, &config_common_file_fops);

	generate_common_debugfs_tree(boardid_common_root);

#ifdef CONFIG_HUAWEI_GPIO_UNITE
	gpio_file = debugfs_create_file("gpio", 0444, boardid_debugfs_root, NULL, &config_gpio_fops);
#endif
	return 0;

}
