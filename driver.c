//
// p3cache
//	enables L2 cache on PCs with buggy BIOSes,
//	eg. Thinkpad 600E upgraded to Pentium III
//
//
// based on pIIIcache Linux Kernel 2.6 module by
// 	Damien Challet <challet@maths.ox.ac.uk>
// which was based on modified code from LinuxBIOS project
//
// BeOS port by
// Maciej Witkowiak <ytm@elysium.pl>
// 2006.01.04
//
// if something doesn't work - uncomment the next line and checkout
// /boot/home/p3cache.log file

//#define DEBUGLOG

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <printf.h>

int32 api_version = B_CUR_DRIVER_API_VERSION;

#ifdef DEBUGLOG
	#define LOG(x)	sprintf(logbuff, x); write(logfd, logbuff, strlen(logbuff));
	#define printk(x) LOG(x)
	#define printki(x,y) sprintf(logbuff, x, y); write(logfd, logbuff, strlen(logbuff));
	#define printkii(x,y,z) sprintf(logbuff, x, y, z); write(logfd, logbuff, strlen(logbuff));

	static char logbuff[256];
	int logfd = -1;	// logfile
#else
	#define LOG(x)
	#define printk(x)
	#define printki(x,y)
	#define printkii(x,y,z)
#endif

/* ----------
	init_hardware - called once the first time the driver is loaded
----- */
status_t
init_hardware (void)
{
#ifdef DEBUGLOG
	logfd = open("/boot/home/p3cache.log", O_CREAT|O_WRONLY);
#endif
	p6_configure_l2_cache();

	return B_OK;
}


/* ----------
	init_driver - optional function - called every time the driver
	is loaded.
----- */
status_t
init_driver (void)
{
	return B_OK;
}

/* ----------
	uninit_driver - optional function - called every time the driver
	is unloaded
----- */
void
uninit_driver (void)
{
#ifdef DEBUGLOG
	close(logfd);
#endif
}

	
/* ----------
	p3cache_open - handle open() calls
----- */

static status_t
p3cache_open (const char *name, uint32 flags, void** cookie)
{
	return B_OK;
}

/* ----------
	p3cache_read - handle read() calls
----- */


static status_t
p3cache_read (void* cookie, off_t position, void *buf, size_t* num_bytes)
{
	int i;
	*num_bytes = 0;				/* tell caller nothing was read */
	return B_IO_ERROR;
}


/* ----------
	p3cache_write - handle write() calls
----- */

static status_t
p3cache_write (void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	*num_bytes = 0;				/* tell caller nothing was written */

	LOG("write triggers cache enable\n");
	p6_configure_l2_cache();
	return B_IO_ERROR;
}


/* ----------
	p3cache_control - handle ioctl calls
----- */

static status_t
p3cache_control (void* cookie, uint32 op, void* arg, size_t len)
{
	return B_BAD_VALUE;
}


/* ----------
	p3cache_close - handle close() calls
----- */

static status_t
p3cache_close (void* cookie)
{
	return B_OK;
}


/* -----
	p3cache_free - called after the last device is closed, and after
	all i/o is complete.
----- */
static status_t
p3cache_free (void* cookie)
{
	return B_OK;
}


/* -----
	null-terminated array of device names supported by this driver
----- */

static const char *p3cache_name[] = {
	"misc/p3cache",
	NULL
};

/* -----
	function pointers for the device hooks entry points
----- */

device_hooks p3cache_hooks = {
	p3cache_open, 			/* -> open entry point */
	p3cache_close, 			/* -> close entry point */
	p3cache_free,			/* -> free cookie */
	p3cache_control, 		/* -> control entry point */
	p3cache_read,			/* -> read entry point */
	p3cache_write			/* -> write entry point */
};

/* ----------
	publish_devices - return a null-terminated array of devices
	supported by this driver.
----- */

const char**
publish_devices()
{
	return p3cache_name;
}

/* ----------
	find_device - return ptr to device hooks structure for a
	given device name
----- */

device_hooks*
find_device(const char* name)
{
	return &p3cache_hooks;
}

///////////////////////
//
// end of BeOS driver-specific stuff, now general code from pIIIcache Linux

// l2_cache.h

#define EBL_CR_POWERON	0x2A

#define BBL_CR_D0	0x88
#define BBL_CR_D1	0x89
#define BBL_CR_D2	0x8A
#define BBL_CR_D3	0x8B

#define BBL_CR_ADDR	0x116
#define BBL_CR_DECC	0x118
#define BBL_CR_CTL	0x119
#define BBL_CR_TRIG	0x11A
#define BBL_CR_BUSY	0x11B
#define BBL_CR_CTL3	0x11E

// msr.h

#define rdmsr(msr,val1,val2) \
       __asm__ __volatile__("rdmsr" \
			    : "=a" (val1), "=d" (val2) \
			    : "c" (msr))

#define wrmsr(msr,val1,val2) \
     __asm__ __volatile__("wrmsr" \
			  : /* no outputs */ \
			  : "c" (msr), "a" (val1), "d" (val2))

// cpuid.h

static inline void cpuid(int op, int *eax, int *ebx, int *ecx, int *edx)
{
        __asm__("pushl %%ebx\n\t"
		"cpuid\n\t"
		"movl	%%ebx, %%esi\n\t"
		"popl	%%ebx\n\t"
                : "=a" (*eax),
                  "=S" (*ebx),
                  "=c" (*ecx),
                  "=d" (*edx)
                : "a" (op)
                : "cc");
}

// pIIIcache.c

int signal_l2(unsigned int address_high, unsigned int address_low,
	      unsigned int data_high, unsigned int data_low,
	      int way, int command)
{
	unsigned int eax, edx;
	int i;

	/* Write L2 Address to BBL_CR_ADDR */
	wrmsr(0x116, address_low, address_high);

	/* Write data to BBL_CR_D[0-3] */
	for (i = 0; i < 4; i++)
		wrmsr(0x88 + i, data_low, data_high);

	/* Put the command into BBL_CR_CTL */
	rdmsr(0x119, eax, edx);
	edx = 0;
	eax &= 0xfffffce0;
	eax |= command;
	eax |= way << 8;
	wrmsr(0x119, eax, edx);

	/* Write to BBL_CR_TRIG to trigger L2 controller */
	wrmsr(0x11a, 0, 0);

	/* Poll the controller to see when done */
	for (i = 0; i < 0x100; i++) {
		/* Read BBL_CR_BUSY */
		rdmsr(0x11b, eax, edx);
		/* If not busy then return */
		if ((eax & 1) == 0)
			return 0;
	}

	/* Return timeout code */
	return -1;
}

/* Read the L2 Cache controller register at given address */
static int read_l2(unsigned int address)
{
	unsigned int eax, edx;

	address = address << 5;

	/* Send a L2 Control Register Read to L2 controller */
	if (signal_l2(0, address, 0, 0, 0, 2) != 0)
		return -1;

	/* If OK then get the result from BBL_CR_ADDR */
	rdmsr(0x116, eax, edx);

	eax = eax >> 0x15;

	return eax;
}

/* data1 = eax, data2 = ebx */
static int write_l2_2(unsigned int address,
		      unsigned int data1, unsigned int data2)
{
	data1 &= 0xff;
	data1 = data1 << 0x15;

	data2 = data2 << 0x0b;
	data2 &= 0x1800;
	data1 |= data2;

	data2 = data2 << 6;
	data2 &= 0x20000;
	data1 |= data2;

	address = address << 5;
	address |= data1;

	/* L2 Control Register Write */
	return signal_l2(0, address, 0, 0, 0, 3);
}

/* Write data into the L2 controller register at address */
static int write_l2(unsigned int address, int data)
{
	int v1, v2, i;

	v1 = read_l2(0);
	if (v1 < 0)
		return -1;

	v2 = read_l2(2);
	if (v2 < 0)
		return -1;

	if ((v1 & 0x20) == 0) {
		v2 &= 0x3;
		v2++;
	} else
		v2 &= 0x7;

	for (i = 0; i < v2; i++)
		if (write_l2_2(address, data, i) != 0)
			return -1;

	return 0;
}

void cache_disable(void)
{
	unsigned int tmp;

	/* Disable cache */
	printk("L2 Cache disabled\n");

	/* Write back the cache and flush TLB */
	asm volatile ("movl  %%cr0, %0\n\t"
		      "orl  $0x40000000, %0\n\t"
		      "wbinvd\n\t"
		      "movl  %0, %%cr0\n\t"
		      "wbinvd\n\t"
		      : "=r" (tmp) : : "memory");
}

void cache_enable(void)
{
	unsigned int tmp;

	asm volatile ("movl  %%cr0, %0\n\t"
		      "andl  $0x9fffffff, %0\n\t"
		      "movl  %0, %%cr0\n\t"
		      :"=r" (tmp) : : "memory");

	printk("L2 Cache enabled\n");
}

/* Latency Tables */
struct latency_entry {
	unsigned char key;
	unsigned char value;
};

static const struct latency_entry latency_650_t0[] = {
	{0x10, 0x02}, {0x50, 0x02}, {0x20, 0x04}, {0x60, 0x06},
	{0x00, 0x08}, {0x40, 0x0C}, {0x12, 0x06}, {0x52, 0x0A},
	{0x22, 0x0E}, {0x62, 0x10}, {0x02, 0x10}, {0xFF, 0x00}
};

static const struct latency_entry latency_650_t1[] = {
	{0x12, 0x14}, {0x52, 0x16}, {0x22, 0x16}, {0x62, 0x16},
	{0xFF, 0x00}
};

static const struct latency_entry latency_670_t0[] = {
	{0x60, 0x06}, {0x00, 0x08}, {0x12, 0x06}, {0x52, 0x0A},
	{0x22, 0x0E}, {0x62, 0x10}, {0x02, 0x10}, {0x42, 0x02},
	{0x11, 0x0E}, {0x51, 0x0C}, {0x21, 0x02}, {0x61, 0x10},
	{0x01, 0x10}, {0x41, 0x02}, {0xFF, 0x00}
};

static const struct latency_entry latency_670_t1[] = {
	{0x22, 0x18}, {0x62, 0x18}, {0x02, 0x1A}, {0x11, 0x18},
	{0xFF, 0x00}
};

static const struct latency_entry latency_670_t2[] = {
	{0x22, 0x12}, {0x62, 0x14}, {0x02, 0x16}, {0x42, 0x1E},
	{0x11, 0x12}, {0x51, 0x16}, {0x21, 0x1E}, {0x61, 0x14},
	{0x01, 0x16}, {0x41, 0x1E}, {0xFF, 0x00}
};

/* Latency tables for 650 model/type */
static const struct latency_entry *latency_650[] = {
	latency_650_t0, latency_650_t1, latency_650_t1
};

/* Latency tables for 670 model/type */
static const struct latency_entry *latency_670[] = {
	latency_670_t0, latency_670_t1, latency_670_t2
};

static int calculate_l2_latency(void)
{
	unsigned int eax, ebx, ecx, edx;
	int l;
	const struct latency_entry *latency_table, *le;

	/* Read an undocumented MSR */
	rdmsr(0x17, eax, edx);

	printkii("rdmsr(0x17) = %x, %x\n", eax, edx);

	/* Mask out [23:20] in EDX. Are Intel trying to hide this?? */
	edx &= 0x1e00000;

	if (edx != 0) {
		/* Get latency value directly from edx */
		l = edx >> 20;
	} else {
		/* Look up the latency from tables */
		int t, a;

		/* Read L2 register 0 */
		a = read_l2(0);
		if (a < 0)
			return -1;

		/* Mask out [7:4] */
		a &= 0xf0;

		if ((a & 0x20) == 0)
			t = 0;
		else if (a == 0x20)
			t = 1;
		else if (a == 0x30)
			t = 2;
		else
			return -1;

		cpuid(1, &eax, &ebx, &ecx, &edx);

		/* Mask out Model/Type */
		eax &= 0xfff0;

		printki("L2 latency type = %x\n", t);

		if (eax == 0x650) {
			/* Read EBL_CR_POWERON */
			rdmsr(0x2a, eax, edx);

			/* Mask [25:22] [19] Clock frequency Ratio & reserved */
			eax &= 0x3c80000;

			/* Shift to [7:4] [1] */
			eax >>= 18;

			latency_table = latency_650[t];
		} else if (eax == 0x670) {
			/* Read EBL_CR_POWERON */
			rdmsr(0x2a, eax, edx);

			/* Mask [25:22] [19:18] Clock frequency Ratio & reserved */
			eax &= 0x3cc0000;

			/* Shift to [7:4] [1:0] */
			eax >>= 18;

			latency_table = latency_670[t];
		} else
			return -1;

		printki("Searching for key %x\n", eax);

		/* Search table for matching entry */
		for (le = latency_table; le->key != eax; le++) {
			/* Fail if we get to the end of the table */
			if (le->key == 0xff) {
				printki(
				       "Could not find key %x in latency table\n",
				       eax);
				return -1;
			}
		}

		l = le->value;
	}

	printki("L2 Cache latency is %d\n", l / 2);

	/* Read BBL_CR_CTL3 */
	rdmsr(0x11e, eax, edx);

	/* Mask out the latency */
	eax &= 0xffffffe1;

	/* Or in the new value */
	eax |= l;

	/* Write BBL_CR_CTL3 */
	wrmsr(0x11e, eax, edx);

	return 0;
}

/* Pass some infomation into the L2 controller register 4*/
static int set_l2_register4(int l)
{
	int a;

	/* Mask out all but lower 2 bits in l */
	l &= 3;

	/* Read L2 register 4 */
	a = read_l2(4);
	if (a < 0)
		return -1;

	/* Mask off the lower 2 bits */
	a &= 0xfffc;

	/* Or in the new latency */
	a |= l;

	/* Write L2 register 4 */
	if (write_l2(4, a) != 0)
		return -1;

	return 0;
}

/* Write data_high:data_low into the cache at address1. Test address2
 * to see if the same data is returned. Return 0 if the data matches.
 * return lower 16 bits if mismatched data if mismatch. Return -1
 * on error
 */
static int test_l2_address_alias(unsigned int address1,
				 unsigned int address2,
				 unsigned int data_high,
				 unsigned int data_low)
{
	unsigned int eax, edx;
	int d;

	/* Tag Write with Data Write for L2 */
	if (signal_l2(0, address1, data_high, data_low, 0, 0x1c) != 0)
		return -1;

	/* Tag Read with Data Read for L2 */
	if (signal_l2(0, address2, 0, 0, 0, 0xe) != 0)
		return -1;

	/* Read data from BBL_CR_D[0-3] */
	for (d = 0x88; d <= 0x8b; d++) {
		rdmsr(d, eax, edx);

		if (eax != data_low || edx != data_high)
			return (eax & 0xffff);
	}

	return 0;
}

static int calculate_l2_cache_size(void)
{
	unsigned int eax, edx;
	int v;

	v = read_l2(0);
	if (v < 0)
		return -1;
	if ((v & 0x20) == 0) {
		unsigned int cache_setting;
		int v;
		unsigned int address, size;

		/* Read BBL_CR_CTL3 */
		rdmsr(0x11e, eax, edx);

		/* Check sizes from 256KB up to 4MB */
		for (cache_setting = 0x2000;
		     cache_setting <= 0x20000; cache_setting <<= 1) {
			unsigned int new_eax;

			/* Mask out the cache size */
			eax &= 0xfffc1fff;

			/* Or in the current setting */
			eax |= cache_setting;

			new_eax = eax;

			/* Write new value into BBL_CR_CTL3 and read it back */
			wrmsr(0x11e, eax, edx);
			rdmsr(0x11e, eax, edx);

			/* If the value didn't change then break */
			if (eax != new_eax)
				break;
		}

		/* Reset to the last value that worked */
		cache_setting >>= 1;

		/* Mask to just the cache setting bits */
		cache_setting &= 0x3e000;

		/* Mask out cache size from eax */
		eax &= 0xfffc1fff;

		/* Or in the correct setting */
		eax |= cache_setting;

		/* Write new value into BBL_CR_CTL3 */
		wrmsr(0x11e, eax, edx);

		printki("Maximum cache mask is %x\n", cache_setting);

		/* Write aaaaaaaa:aaaaaaaa to address 0 in the l2 cache */
		v = test_l2_address_alias(0, 0, 0xaaaaaaaa, 0xaaaaaaaa);
		if (v != 0)
			return -1;

		size = 1;
		address = 0x8000;

		while (1) {
			v =
			    test_l2_address_alias(address, 0, 0x55555555,
						  0x55555555);
			if (v < 0)
				return -1;
			else if (v == 0)
				break;

			size <<= 1;
			address <<= 1;

			if (address > 0x200000)
				return -1;
		}

		/* Mask size */
		size &= 0x3e;

		/* Shift to [17:13] */
		size <<= 12;

		/* Read BBL_CR_CTL3 */
		rdmsr(0x11e, eax, edx);

		/* Mask out the cache_setting */
		eax &= 0xfffc1fff;

		/* Or in the calculated value */
		eax |= size;

		/* Write cache size into BBL_CR_CTL3 */
		wrmsr(0x11e, eax, edx);

		printki("L2 Cache Mask is %x\n", size);

		/* Shift to [6:2] */
		size >>= 11;

		v = read_l2(2);

		printki("read_l2(2) = %x\n", v);

		if (v < 0)
			return -1;

		v &= 0x3;

		/* Shift size right by v */
		size >>= v;

		/* Or in this size */
		v |= size;

		printki("write_l2(2) = %x\n", v);

		if (write_l2(2, v) != 0)
			return -1;
	} else {
		int a, b, c;

		a = read_l2(2);

		printki("read_l2(2) = %x\n", a);

		if (a < 0)
			return -1;

		b = a & 0x7;
		c = a >> 6;

		a = 1 << c * b;

		a &= 0xf;

		printki("Calculated a = %x\n", a);

		if (a == 0)
			return -1;

		/* Shift to 17:14 */
		a <<= 14;

		/* Read BBL_CR_CTL3 */
		rdmsr(0x11e, eax, edx);

		/* Mask out the cache */
		eax &= 0xfffc1fff;

		/* Or in the calculated value */
		eax |= a;

		/* Write cache size into BBL_CR_CTL3 */
		wrmsr(0x11e, eax, edx);
	}

	return 0;
}

static int calculate_l2_physical_address_range(void)
{
	unsigned int eax, edx;
	int r0, r3;

	r3 = read_l2(3);
	if (r3 < 0)
		return -1;

	r0 = read_l2(0);
	if (r0 < 0)
		return -1;

	if (r0 & 0x20)
		r3 = 0x7;
	else
		r3 &= 0x7;

	printki("L2 Physical Address Range is %dM\n", (1 << r3) * 512);

	/* Shift into [22:20] */
	r3 = r3 << 20;

	/* Read BBL_CR_CTL3 */
	rdmsr(0x11e, eax, edx);

	/* Mask out [22:20] */
	eax &= 0xff8fffff;

	/* Or in the new value */
	eax |= r3;

	/* Write BBL_CR_CTL3 */
	wrmsr(0x11e, eax, edx);

	return 0;
}

static int calculate_l2_ecc(void)
{
	unsigned int eax, edx;
	const unsigned int data1 = 0xaa55aa55;
	const unsigned int data2 = 0xaaaaaaaa;

	/* Set User Supplied ECC in BBL_CR_CTL */
	rdmsr(0x119, eax, edx);
	eax |= 0x40000;
	wrmsr(0x119, eax, edx);

	/* Write a value into the L2 Data ECC register BBL_CR_DECC */
	wrmsr(0x118, data1, 0);

	if (test_l2_address_alias(0, 0, data2, data2) < 0)
		return -1;

	/* Read back ECC from BBL_CR_DECC */
	rdmsr(0x118, eax, edx);

	if (eax == data1) {
		printk("L2 ECC Checking is enabled\n");

		/* Set ECC Check Enable in BBL_CR_CTL3 */
		rdmsr(0x11e, eax, edx);
		eax |= 0x20;
		wrmsr(0x11e, eax, edx);
	}

	/* Clear User Supplied ECC in BBL_CR_CTL */
	rdmsr(0x119, eax, edx);
	eax &= 0xfffbffff;
	wrmsr(0x119, eax, edx);

	return 0;
}

int
p6_configure_l2_cache()
{
	unsigned int eax, ebx, ecx, edx;
	int signature, tmp;
	int cache_size;
	int result;

	printk("Configuring L2 cache...\n");
	cpuid(0, &eax, &ebx, &ecx, &edx);

	if (ebx != 0x756e6547 || edx != 0x49656e69 || ecx != 0x6c65746e) {
		printk( "Not 'GenuineIntel' Processor\n");
		goto bad;
	}
	cpuid(1, &eax, &ebx, &ecx, &edx);

	/* Mask out the stepping */
	signature = eax & 0xfff0;
	if (signature & 0x1000) {
		printk("Overdrive chip no L2 cache configuration\n");
		goto done;
	}

	if (signature < 0x630 || signature > 0x680) {
		printki("CPU signature of %x so no L2 cache configuration\n",
		     signature);
		goto done;
	}

	/* Read BBL_CR_CTL3 */
	rdmsr(BBL_CR_CTL3, eax, edx);

	/* If bit 23 (L2 Hardware disable) is set then done */
	if (eax & 0x800000) {
		printk("L2 Hardware disabled\n");
		goto done;
	}

	if (signature == 0x630) {
		/* 0x630 signature setup */

		/* Read EBL_CR_POWERON */
		rdmsr(EBL_CR_POWERON, eax, edx);

		/* Mask out [22-24] Clock frequency ratio */
		eax &= 0x1c00000;
		if (eax == 0xc00000 || eax == 0x1000000) {
			printki( "Incorrect clock frequency ratio %x\n",
			       eax);
			goto bad;
		}

		cache_disable();
		/* Read BBL_CR_CTL3 */
		rdmsr(BBL_CR_CTL3, eax, edx);
		/* Mask out:
		 * [0] L2 Configured
		 * [5] ECC Check Enable
		 * [6] Address Parity Check Enable
		 * [7] CRTN Parity Check Enable
		 * [8] L2 Enabled
		 * [12:11] Number of L2 banks
		 * [17:13] Cache size per bank
		 * [18] Cache state error checking enable
		 * [22:20] L2 Physical Address Range Support
		 */
		eax &= 0xff88061e;

		/* Set:
		 * [17:13] = 00010 = 512Kbyte Cache size per bank
		 * [18] Cache state error checking enable
		 */
		eax |= 0x44000;
		/* Write BBL_CR_CTL3 */
		wrmsr(BBL_CR_CTL3, eax, edx);
	} else {
		int calc_eax;
		int v;

		/* After 0x630 signature setup */

		/* Read EBL_CR_POWERON */
		rdmsr(0x2a, eax, edx);

		/* Mask out [22-24] Clock frequency ratio */
		eax &= 0x3c00000;
		if (eax == 0xc00000 || eax == 0x3000000) {
			printki( "Incorrect clock frequency ratio %x\n",
			       eax);
			goto bad;
		}

		cache_disable();

		/* Read BBL_CR_CTL3 */
		rdmsr(0x11e, eax, edx);

		/* Mask out:
		 * [0] L2 Configured
		 * [5] ECC Check Enable
		 * [6] Address Parity Check Enable
		 * [7] CRTN Parity Check Enable
		 * [8] L2 Enabled
		 * [12:11] Number of L2 banks
		 * [17:13] Cache size per bank
		 * [18] Cache state error checking enable
		 * [22:20] L2 Physical Address Range Support
		 */
		eax &= 0xff88061e;
		/* Set:
		 * [17:13] = 00000 = 128Kbyte Cache size per bank
		 * [18] Cache state error checking enable
		 */
		eax |= 0x40000;

		/* Write BBL_CR_CTL3 */
		wrmsr(0x11e, eax, edx);

		/* Set the l2 latency in BBL_CR_CTL3 */
		if (calculate_l2_latency() != 0)
			goto bad;

		/* Read the new latency values back */
		rdmsr(0x11e, calc_eax, edx);

		/* Write back the original default value */
		wrmsr(0x11e, eax, edx);

		/* Mask [27:26] out of BBL_CR_CTL3 - Reserved?? */
		v = calc_eax & 0xc000000;

		/* Shift to [1:0] */
		v >>= 26;

		printki("Sending %x to set_l2_register4\n", v);
		if (set_l2_register4(v) != 0)
			goto bad;

		/* Restore the correct latency value into BBL_CR_CTL3 */
		wrmsr(0x11e, calc_eax, edx);
	}

	/* Read L2 register 0 */
	tmp = read_l2(0);
	if (tmp < 0) {
		printk("Failed to read_l2(0)\n");
		goto bad;
	}

	/* test if L2(0) has bit 0x20 set */
	if ((tmp & 0x20) != 0) {
		/* Read BBL_CR_CTL3 */
		rdmsr(0x11e, eax, edx);
		/* Set bits [6-7] CRTN + Address Parity enable */
		eax |= 0xc0;
		/* Write BBL_CR_CTL3 */
		wrmsr(0x11e, eax, edx);
	}

	if (calculate_l2_ecc() != 0) {
		printk( "Failed to calculate L2 ECC\n");
		goto bad;
	}

	if (calculate_l2_physical_address_range() != 0) {
		printk(
		       "Failed to calculate L2 physical address range\n");
		goto bad;
	}

	if (calculate_l2_cache_size() != 0) {
		printk("Failed to calculate L2 cache size\n");
		goto bad;
	}

	/* Turn on cache. Only L1 is active at this time. */
	cache_enable();

	/* Get the calculated cache size from BBL_CR_CTL3 [17:13] */
	rdmsr(0x11e, eax, edx);
	cache_size = (eax & 0x3e000);
	if (cache_size == 0)
		cache_size = 0x1000;
	cache_size = cache_size << 3;

	/* Cache is 4 way for each address */
	printki("L2 Cache size is %dK\n", cache_size * 4 / 1024);

	/* Write to all cache lines to initialize */
	while (cache_size > 0) {
		int way;

		/* Each Cache line in 32 bytes */
		cache_size -= 0x20;

		/* Update each way */
		for (way = 0; way < 4; way++) {
			/* Send Tag Write w/Data Write (TWW) to L2 controller 
			 * MESI = Invalid
			 */
			if (signal_l2(0, cache_size, 0, 0, way, 0x1c) != 0) {
				printkii(
				       "Failed on signal_l2(%x, %x)\n",
				       cache_size, way);
				goto bad;
			}
		}
	}
	printk("L2 Cache lines initialized\n");

	/* Disable cache */
	cache_disable();

	/* Set L2 cache configured in BBL_CR_CTL3 */
	rdmsr(0x11e, eax, edx);
	eax |= 0x1;
	wrmsr(0x11e, eax, edx);

	/* Invalidate cache and discard unsaved writes */
	asm volatile ("invd");

	/* Write 0 to L2 control register 5 */
	if (write_l2(5, 0) != 0) {
		printk("write_l2(5, 0) failed\n");
		goto done;
	}

	if (signature == 0x650 || signature == 0x670) {
		/* Change the L2 latency to 0101 then back to 
		 * original value. I don't know why this is needed - dpd
		 */
		int old_eax;
		rdmsr(0x11e, eax, edx);
		old_eax = eax;
		eax &= 0xffffffe1;
		eax |= 0x0000000a;
		wrmsr(0x11e, eax, edx);
		wrmsr(0x11e, old_eax, edx);
	}

	/* Set L2 enabled in BBL_CR_CTL3 */
	rdmsr(0x11e, eax, edx);
	eax |= 0x00000100;
	wrmsr(0x11e, eax, edx);

	/* Turn on cache. Both L1 and L2 are now active. Wahoo! */
done:
	result = 0;
 out:
	cache_enable();

 	printk("done.\n");
	return result;
bad: 
	result = -1;
	goto out;
}
