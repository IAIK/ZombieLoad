#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jo Van Bulck <jo.vanbulck@cs.kuleuven.be>");
MODULE_DESCRIPTION("Module to bypass CONFIG_STRICT_DEVMEM kernel compilation option");

#define DEV           "devmem-mod"
#define log(msg, ...) printk(KERN_INFO  "[" DEV "] " msg "\n", ##__VA_ARGS__)
#define err(msg, ...) printk(KERN_ALERT "[" DEV "] error: " msg "\n", ##__VA_ARGS__)

/* Code from: <https://www.libcrack.so/2012/09/02/bypassing-devmem_is_allowed-with-kprobes/>
              <https://github.com/jovanbulck/sgx-step> */
static int devmem_is_allowed_handler (struct kretprobe_instance *rp, struct pt_regs *regs)
{
    log("intercepted devmem_is_allowed!");

    if (regs->ax == 0)
    {
        regs->ax = 0x1;
    }
    return 0;
}

static struct kretprobe krp = {
    .handler = devmem_is_allowed_handler,
    .maxactive = 20 /* Probe up to 20 instances concurrently. */
};

int init_module(void)
{
    /* Activate a kretprobe to bypass CONFIG_STRICT_DEVMEM kernel compilation option */
    krp.kp.symbol_name = "devmem_is_allowed";
    if (register_kretprobe(&krp) < 0)
    {
        err("register_kprobe failed..");
        return -EINVAL;
    }

    log("successfully registered devmem_is_allowed kretprobe!");
    return 0;
}

void cleanup_module(void)
{
    unregister_kretprobe(&krp);
    log("kernel module unloaded");
}
