/*
 * Copyright © 2017-2021 Samuel Holland <samuel@sholland.org>
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/sysfs.h>

#include <asm/arch_timer.h>
#include <asm/sysreg.h>

#define SYSREG_ATTR_RW(_reg) \
static ssize_t _reg##_show(struct kobject *kobj, struct kobj_attribute *attr, \
			   char *buf) \
{ \
	return sprintf(buf, "%016llx\n", read_sysreg(_reg)); \
} \
static void _reg##_write(void *value) \
{ \
	u64 val = *(u64 *)value; \
	write_sysreg(val, _reg); \
} \
static ssize_t _reg##_store(struct kobject *kobj, struct kobj_attribute *attr, \
			    const char *buf, size_t count) \
{ \
	u64 val; \
	sscanf(buf, "%llx", &val); \
	on_each_cpu(_reg##_write, &val, true); \
	return count; \
} \
static const struct kobj_attribute sysreg_attr_##_reg = __ATTR_RW(_reg)

#define SYSREG_ATTR_REF(_reg) &sysreg_attr_##_reg.attr

SYSREG_ATTR_RW(cntkctl_el1);
SYSREG_ATTR_RW(cntp_ctl_el0);
SYSREG_ATTR_RW(cntp_cval_el0);
SYSREG_ATTR_RW(cntp_tval_el0);
SYSREG_ATTR_RW(cntpct_el0);
SYSREG_ATTR_RW(cntv_ctl_el0);
SYSREG_ATTR_RW(cntv_cval_el0);
SYSREG_ATTR_RW(cntv_tval_el0);
SYSREG_ATTR_RW(cntvct_el0);

static const struct attribute *arch_timer_control_attrs[] = {
	SYSREG_ATTR_REF(cntkctl_el1),
	SYSREG_ATTR_REF(cntp_ctl_el0),
	SYSREG_ATTR_REF(cntp_cval_el0),
	SYSREG_ATTR_REF(cntp_tval_el0),
	SYSREG_ATTR_REF(cntpct_el0),
	SYSREG_ATTR_REF(cntv_ctl_el0),
	SYSREG_ATTR_REF(cntv_cval_el0),
	SYSREG_ATTR_REF(cntv_tval_el0),
	SYSREG_ATTR_REF(cntvct_el0),
	NULL
};

static struct kobject *kobj;

static void arch_timer_control_exit(void)
{
	kobject_put(kobj);
}
module_exit(arch_timer_control_exit);

static int __init arch_timer_control_init(void)
{
	const struct arch_timer_erratum_workaround *wa;
	int ret;

	kobj = kobject_create_and_add("arch_timer", kernel_kobj);
	if (!kobj)
		return -ENOMEM;

	ret = sysfs_create_files(kobj, arch_timer_control_attrs);
	if (ret) {
		kobject_put(kobj);
		return ret;
	};

	wa = this_cpu_read(timer_unstable_counter_workaround);
	pr_info("Current workaround: %s (%pS)\n",
		wa ? wa->desc : "none", wa);

	return 0;
}
module_init(arch_timer_control_init);

MODULE_LICENSE("GPL");
