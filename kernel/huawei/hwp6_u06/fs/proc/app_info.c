#include <linux/types.h>
#include <linux/proc_fs.h>
#include <asm/setup.h>
#include <linux/pagemap.h>

struct st_read_proc {
	char *name;
	int (*read_proc)(char *, char **, off_t, int, int *, void *);
};

extern unsigned int get_pd_charge_flag(void);
extern unsigned int resetmode_is_normal(void);

extern unsigned int get_boot_into_recovery_flag(void);
/* same as in proc_misc.c */
static int proc_calc_metrics(char *page, char **start, off_t off,
				 int count, int *eof, int len)
{
	if (len <= off + count)
		*eof = 1;
	*start = page + off;
	len -= off;
	if (len > count)
		len = count;
	if (len < 0)
		len = 0;
	return len;
}

static int app_tag_read_proc(char *page, char **start, off_t off,
				 int count, int *eof, void *data)
{
	int len = 0;
	u32 charge_flag = 0;
	u32 recovery_flag = 0;
	u32 reset_normal_flag = 0;

	recovery_flag = get_boot_into_recovery_flag();

	charge_flag = get_pd_charge_flag();
	reset_normal_flag = resetmode_is_normal();

	len = snprintf(page, PAGE_SIZE,
					"recovery_flag:\n%d\n"
					"charge_flag:\n%d\n"
					"reset_normal_flag:\n%d\n",
					recovery_flag,
					charge_flag,
					reset_normal_flag);

	return proc_calc_metrics(page, start, off, count, eof, len);
}

static struct st_read_proc simple_ones[] = {
			{"app_info", app_tag_read_proc},
			{NULL,}
		};

void __init proc_app_info_init(void)
{
	struct st_read_proc *p;

	for (p = simple_ones; p->name; p++)
		create_proc_read_entry(p->name, 0, NULL, p->read_proc, NULL);
}
