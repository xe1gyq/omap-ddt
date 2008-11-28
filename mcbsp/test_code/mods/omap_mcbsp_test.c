/*
 * omap2_mcbsp_test.c
 *
 * Test Driver for OMAP2 McBSP driver
 *
 * Copyright (C) 2006 Texas Instruments, Inc.
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 *
 * Author : Syed Mohammed Khasim
 * Date   : 01 March 2006
 */

//#include <linux/config.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/slab.h>
#include <linux/rtnetlink.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/proc_fs.h>
#include <linux/version.h>

#include <asm/irq.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/serial.h>
#include <asm/dma.h>
#include <asm/arch/omap2_mcbsp.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,00))
#include <linux/dma-mapping.h>
#endif

static int start_mcbsp_transmission(void);
struct mcbsp_info_struct
{
	int mcbsp_id;
	int mode; /* Master or Slave */
	dma_addr_t tx_buf_dma_phys;
	unsigned int tx_buf_dma_virt;
	dma_addr_t rx_buf_dma_phys;
	unsigned int rx_buf_dma_virt;
	int rx_cnt;
	int tx_cnt;
	int num_of_tx_transfers;
	int num_of_rx_transfers;
	int send_cnt;
	int recv_cnt;
	spinlock_t rx_lock;
	spinlock_t tx_lock;

};
static struct mcbsp_info_struct mcbsptest_info;

/* Module parameters are initialized to default values */

static int buffer_size   = 64*4;
static int no_of_trans   = 100;
static int no_of_tx      = 0;
static int no_of_rx      = 0;
static int sample_rate   = 8000;
static int phase         = OMAP2_MCBSP_FRAME_SINGLEPHASE;
static int clkr_polarity = OMAP2_MCBSP_CLKR_POLARITY_RISING;
static int fsr_polarity  = OMAP2_MCBSP_FS_ACTIVE_LOW;
static int clkx_polarity = OMAP2_MCBSP_CLKX_POLARITY_RISING;
static int fsx_polarity  = OMAP2_MCBSP_FS_ACTIVE_LOW;
static int justification = OMAP2_MCBSP_RJUST_ZEROMSB;
static int word_length   = OMAP2_MCBSP_WORDLEN_32;
static int test_mcbsp_id = OMAP2_MCBSP_INTERFACE3;
static int words_per_frame = 0;
long int tx_sec=0,tx_usec=0;

/*MODULE_PARM( buffer_size, "i");
MODULE_PARM( no_of_trans, "i");
MODULE_PARM( no_of_tx      , "i");
#MODULE_PARM( no_of_rx      , "i");
#MODULE_PARM( sample_rate   , "i");
#MODULE_PARM( phase         , "i");
#MODULE_PARM( clkr_polarity , "i");
#MODULE_PARM( fsr_polarity  , "i");
#MODULE_PARM( clkx_polarity , "i");
#MODULE_PARM( fsx_polarity  , "i");
#MODULE_PARM( justification , "i");
#MODULE_PARM( word_length   , "i");
#MODULE_PARM( words_per_frame , "i");
#MODULE_PARM( test_mcbsp_id , "i");

*/

module_param( buffer_size, int,0 );
module_param( no_of_trans,int ,0);
module_param( no_of_tx      ,int ,0);
module_param( no_of_rx      ,int,0);
module_param( sample_rate   ,int,0);
module_param( phase         ,int,0);
module_param( clkr_polarity ,int,0);
module_param( fsr_polarity  ,int ,0);
module_param( clkx_polarity ,int,0);
module_param( fsx_polarity  ,int ,0);
module_param( justification ,int,0);
module_param( word_length   ,int ,0);
module_param( words_per_frame ,int,0);
module_param( test_mcbsp_id ,int,0 );

unsigned int bits_per_sample[6]={8,12,16,20,24,32};
struct timeval tx_start_time, tx_end_time;
/* Proc interface declaration */
static struct proc_dir_entry  *mcbsp_test_dir,*transmission_file,*status_file;
static int file_type[2]={0,1};

/* Proc interface modules */
static int
read_proc_status(char *page, char **start, off_t off, int count,
                          int *eof, void *data)
{
	int len=0;
	char *p = page;
	if (off != 0)
		goto readproc_status_end;

//	MOD_INC_USE_COUNT;

	p += sprintf(p, "\n\n\n\n");
	p += sprintf(p, "==================================================================== \n\n");
	p += sprintf(p, "                          OMAP2 MCBSP TEST STATUS                      \n");
	p += sprintf(p, "==================================================================== \n\n");
	p += sprintf(p, "McBSP ID                                 : %8d \n",test_mcbsp_id);
	p += sprintf(p, "Buffer size (bytes)                      : %8d \n",buffer_size);
	p += sprintf(p, "Number of transfers                      : %8d \n",no_of_trans);
	p += sprintf(p, "No. of buffers transmitted               : %8d \n",no_of_tx);
	p += sprintf(p, "No. of buffers received                  : %8d \n",no_of_rx);
	p += sprintf(p, "Sampling Rate (frequency, hertz)         : %8d \n",sample_rate);
	p += sprintf(p, "Phase [1=Single, 2=Dual]                 : %8d \n",phase);
	p += sprintf(p, "CLKR_Polarity [1=Rising, 2=Falling]      : %8d\n",clkr_polarity);
	p += sprintf(p, "FSR_Polarity  [0=High, 1=Low]            : %8d\n",fsr_polarity);
	p += sprintf(p, "CLKX_Polarity [1=Rising, 2=Falling]      : %8d\n",clkx_polarity);
	p += sprintf(p, "FSX_Polarity  [0=High, 1=Low]            : %8d\n",fsx_polarity);
	p += sprintf(p, "Justification [0-RJUST,1-SRJUST,2-LJUST] : %8d\n",justification);
	p += sprintf(p, "Word Length   [0-8,2-16,5-32]bits        : %8d\n",word_length);
	p += sprintf(p, "Words Per frame [0-1, 1-2]               : %8d\n",words_per_frame);
	p += sprintf(p, "Time taken for transmission              : %08li sec %08li usec\n\n\n",tx_sec,tx_usec);

readproc_status_end:
	len = (p - page);
	*eof = 1;
	if (off >= len) return 0;
	*start = page + off;
	//MOD_DEC_USE_COUNT;
	return min(count, len - (int)off);
}

static int
write_proc_entry(struct file *file, const char *buffer,
                         unsigned long count, void *data)
{
	int len,i;
	char val[10];

	if (!buffer || (count == 0))
		return 0;

	//MOD_INC_USE_COUNT;

	len = (count > 7) ? 7 : count;
	for(i=0;i<len;i++) val[i]=buffer[i];
	val[i]='\0';

	//MOD_DEC_USE_COUNT;

	if (strncmp(val, "start", 4) == 0){
		start_mcbsp_transmission();
	}
	else if (strncmp(val, "stop", 4) == 0){
		omap2_mcbsp_stop_datatx(mcbsptest_info.mcbsp_id);
		omap2_mcbsp_stop_datarx(mcbsptest_info.mcbsp_id);
	}
	else if (strncmp(val, "suspend", 4) == 0){
		omap2_mcbsp_suspend(mcbsptest_info.mcbsp_id);
	}
	else if (strncmp(val, "resume", 4) == 0){
		omap2_mcbsp_resume(mcbsptest_info.mcbsp_id);
	}
	else
		return -EINVAL;

	return count;
}


static int
create_proc_file_entries(void)
{
	if (!(mcbsp_test_dir = proc_mkdir("driver/mcbsp_test", NULL))){
		printk("\n No mem to create proc file \n");
		return -ENOMEM;
	}

	if (!(transmission_file = create_proc_entry("transmission", 0644, mcbsp_test_dir)))
		goto no_transmission;
	transmission_file->data = &file_type[0];

	if (!(status_file = create_proc_entry("status", 0644, mcbsp_test_dir)))
		goto no_status;
        status_file->data = &file_type[0];

	status_file->read_proc  = read_proc_status;
	transmission_file->write_proc = write_proc_entry;

	mcbsp_test_dir->owner = status_file->owner = transmission_file->owner = THIS_MODULE;
	return 0;

no_status:
	remove_proc_entry("status", mcbsp_test_dir);
no_transmission:
	remove_proc_entry("transmission", mcbsp_test_dir);
	return -ENOMEM;
}

static
void remove_proc_file_entries(void)
{
	remove_proc_entry("transmission", mcbsp_test_dir);
	remove_proc_entry("status", mcbsp_test_dir);
}





/* Callback interface modules */
void omap_mcbsp_send_cb(unsigned short int ch_status, void *arg)
{
	//printk("MCBSP_TEST: Tx Callback for interface %d\n", mcbsptest_info.send_cnt++);
	if(no_of_tx <  no_of_trans){
		if(0 != omap2_mcbsp_send_data(mcbsptest_info.mcbsp_id, &mcbsptest_info,
					mcbsptest_info.tx_buf_dma_phys, buffer_size)){
			printk(KERN_ERR "McBSP Test Driver: Master Send data failed \n");
			return;
		}
		no_of_tx++;
	}
	else{
		printk("McBSP Data Transmission (using DMA) is completed successfully \n");
	}
}

void omap_mcbsp_recv_cb(unsigned short int ch_status, void *arg)
{
#ifdef COMP_DATA
	char *ptr;
	int i,k;
#endif

	//printk("MCBSP_TEST: Rx Callback %d\n",mcbsptest_info.recv_cnt++);
	if(no_of_rx <  no_of_trans){
		if(0 != omap2_mcbsp_receive_data(mcbsptest_info.mcbsp_id, &mcbsptest_info,
					mcbsptest_info.rx_buf_dma_phys, buffer_size)){
			printk(KERN_ERR "McBSP Test Driver: Slave Send data failedn");
			return;
		}
		no_of_rx++;
	}
	else{
		do_gettimeofday(&tx_end_time);
		tx_sec = tx_end_time.tv_sec-tx_start_time.tv_sec;
		tx_usec= tx_end_time.tv_usec-tx_start_time.tv_usec;
		if(tx_usec < 0){
			tx_usec = 0;
		}
		printk("McBSP Data Reception (using DMA) is completed successfully \n");
	}

#ifdef COMP_DATA
	ptr = mcbsptest_info.rx_buf_dma_virt;
	for(k=0;k<buffer_size;){
		//printk("%x ",ptr[k]);
		if(ptr[k] != 0xAA) printk(" Error!!! in data %d \n",no_of_rx);
		k=k+1;
	}
#endif

}

static
int configure_mcbsp_interface(void)
{
	/* SRG Reset */
	if(0 != omap2_mcbsp_set_srg(mcbsptest_info.mcbsp_id,OMAP2_MCBSP_SRG_DISABLE)){
		printk(KERN_ERR "McBSP Test Driver: Resetting SRG failed\n");
		return -1;
	}

	/* FSG Reset */
	if(0 != omap2_mcbsp_set_fsg(mcbsptest_info.mcbsp_id,OMAP2_MCBSP_FSG_DISABLE)){
		printk(KERN_ERR "McBSP Test Driver: Resetting FSG failed\n");
		return -1;
	}

	/* Digital Loop back mode */
	if(0 != omap2_mcbsp_datatxrx_mode(mcbsptest_info.mcbsp_id,OMAP2_MCBSP_DLB)){
		printk(KERN_ERR "McBSP Test Driver: Configuring data tx/rx mode failed\n");
		return -1;
	}

	/* Configure SRG */
	if(0 != omap2_mcbsp_srg_cfg(mcbsptest_info.mcbsp_id,sample_rate,bits_per_sample[word_length],OMAP2_MCBSP_SRGCLKSRC_FCLK,0,
							OMAP2_MCBSP_SRG_FREERUNNING,0)){
		printk(KERN_ERR "McBSP Test Driver: Configuring SRG failed\n");
		return -1;
	}

	fsx_polarity = fsr_polarity;
	/* Configure FSG */
	if(0 != omap2_mcbsp_fsync_cfg(mcbsptest_info.mcbsp_id,OMAP2_MCBSP_TXFSYNC_INTERNAL,OMAP2_MCBSP_RXFSYNC_INTERNAL,
					fsx_polarity, fsr_polarity,((bits_per_sample[word_length]*2) - 1),
					(bits_per_sample[word_length]-1),1)){
		printk(KERN_ERR "McBSP Test Driver: Configuring FSYNC failed\n");
		return -1;
	}

	/* SRG Reset */
	if(0 != omap2_mcbsp_set_srg(mcbsptest_info.mcbsp_id,OMAP2_MCBSP_SRG_ENABLE)){
		printk(KERN_ERR "McBSP Test Driver: Resetting SRG failed\n");
		return -1;
	}

	/* FSG Reset */
	if(0 != omap2_mcbsp_set_fsg(mcbsptest_info.mcbsp_id,OMAP2_MCBSP_FSG_ENABLE)){
		printk(KERN_ERR "McBSP Test Driver: Resetting FSG failed\n");
		return -1;
	}
	return 0;
}

static
int configure_mcbsp_tx(void)
{
	omap2_mcbsp_transfer_params tp;
	int k;
	char *ptr;

	/* XRST Reset */
	if(0 != omap2_mcbsp_set_xrst(mcbsptest_info.mcbsp_id,OMAP2_MCBSP_XRST_DISABLE)){
		printk(KERN_ERR "McBSP Test Driver: TX RST failed\n");
		return -1;
	}

	/* Configure tx clk */
	if(0 != omap2_mcbsp_txclk_cfg(mcbsptest_info.mcbsp_id,
				 OMAP2_MCBSP_CLKTXSRC_INTERNAL,clkx_polarity)){
		printk(KERN_ERR "McBSP Test Driver: Configuring tx clk failed\n");
		return -1;
	}

	/* Configure transfer params */
        tp.phase		= phase;
       	tp.data_delay		= 1;
        tp.reverse_compand	= OMAP2_MCBSP_MSBFIRST;
        tp.word_length1 	= word_length;
        tp.word_length2 	= 0;
        tp.frame_length1	= words_per_frame;
       	tp.frame_length2	= 0;
        tp.justification	= justification;
       	tp.callback		= &omap_mcbsp_send_cb;
		tp.skip_alt=OMAP2_MCBSP_SKIP_NONE;
		tp.auto_reset		= OMAP2_MCBSP_AUTO_RST_NONE;
		tp.data_type       = OMAP2_MCBSP_WORDLEN_NONE;

	if(0 != omap2_mcbsp_set_trans_params(mcbsptest_info.mcbsp_id, &tp)){
		printk(KERN_ERR "McBSP Test Driver: Configuring transfer params failed\n");
		return -1;
	}

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,00))
	mcbsptest_info.tx_buf_dma_virt = (void *)
       		consistent_alloc(GFP_KERNEL | GFP_DMA | GFP_ATOMIC, buffer_size, &mcbsptest_info.tx_buf_dma_phys);
#else
	mcbsptest_info.tx_buf_dma_virt = (void *)
	       	dma_alloc_coherent(NULL, buffer_size, &mcbsptest_info.tx_buf_dma_phys,GFP_KERNEL | GFP_DMA);

#endif
	ptr = mcbsptest_info.tx_buf_dma_virt;
	for(k=0;k<256;){
		ptr[k] = 0xAA;
		k=k+1;
	}
	return 1;
}

static
int configure_mcbsp_rx(void)
{
	omap2_mcbsp_transfer_params rp;

	/* RRST */
	if(0 != omap2_mcbsp_set_rrst(mcbsptest_info.mcbsp_id,OMAP2_MCBSP_RRST_DISABLE)){
		printk("\n Receiver Reset failed \n");
	}

	/* Configure Rx clk */
	if(0 != omap2_mcbsp_rxclk_cfg(mcbsptest_info.mcbsp_id,
				OMAP2_MCBSP_CLKRXSRC_EXTERNAL,clkr_polarity)){
		printk(KERN_ERR "McBSP Test Driver: Configuring rx clk failed\n");
		return -1;
	}

	/* Configure receiver params */
        rp.phase		= phase;
        rp.data_delay		= 1;
        rp.reverse_compand	= OMAP2_MCBSP_MSBFIRST;
        rp.word_length1 	= word_length;
        rp.word_length2 	= 0;
        rp.frame_length1	= words_per_frame;
        rp.frame_length2	= 0;
        rp.justification	= justification;
        rp.callback		= &omap_mcbsp_recv_cb;
		rp.skip_alt=OMAP2_MCBSP_SKIP_NONE;
	rp.auto_reset		= OMAP2_MCBSP_AUTO_RST_NONE;
		rp.data_type		= OMAP2_MCBSP_WORDLEN_NONE;

	if(0 != omap2_mcbsp_set_recv_params(mcbsptest_info.mcbsp_id, &rp)){
		printk(KERN_ERR "McBSP Test Driver: Configuring transfer params failed\n");
		return -1;
	}
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,00))
	mcbsptest_info.rx_buf_dma_virt = (void *)
       	    consistent_alloc(GFP_KERNEL | GFP_DMA | GFP_ATOMIC, buffer_size, &mcbsptest_info.rx_buf_dma_phys);
#else
	mcbsptest_info.rx_buf_dma_virt = (void *)
	       	dma_alloc_coherent(NULL, buffer_size, &mcbsptest_info.rx_buf_dma_phys,GFP_KERNEL | GFP_DMA);

#endif
	return 0;
}

/* transmit mode = receive (0) or transmit (1) */
static
int start_mcbsp_transmission(void)
{
	/* Configure the Master, it should generate the FSX and CLKX */
	configure_mcbsp_tx();
	configure_mcbsp_rx();

	mcbsptest_info.rx_cnt       = 0;
	mcbsptest_info.recv_cnt     = 0;

	mcbsptest_info.tx_cnt      = 0;
	mcbsptest_info.send_cnt    = 0;

	if(0 != omap2_mcbsp_send_data(mcbsptest_info.mcbsp_id, &mcbsptest_info,
					mcbsptest_info.tx_buf_dma_phys, buffer_size)){
		printk(KERN_ERR "McBSP Test Driver: Master Send data failed \n");
		return -1;
	}

	if(0 != omap2_mcbsp_receive_data(mcbsptest_info.mcbsp_id, &mcbsptest_info,
					mcbsptest_info.rx_buf_dma_phys, buffer_size)){
		printk(KERN_ERR "McBSP Test Driver: Slave Send data failedn");
		return -1;
	}

	printk (" Start McBSP Tranmitter \n");
	do_gettimeofday(&tx_start_time);
	/* XRST */
	if(0 != omap2_mcbsp_set_xrst(mcbsptest_info.mcbsp_id,OMAP2_MCBSP_XRST_ENABLE)){
		printk("\n Unable to start transmitter \n");
	}

	printk ("\n Start McBSP Receiver \n");
	/* RRST */
	if(0 != omap2_mcbsp_set_rrst(mcbsptest_info.mcbsp_id, OMAP2_MCBSP_RRST_ENABLE)){
		printk("\n Unable to start receiver \n");
	}
	return 0;
}

static
void fill_global_structure()
{
	mcbsptest_info.mcbsp_id = test_mcbsp_id;
	mcbsptest_info.mode     = OMAP2_MCBSP_MASTER ; /* Master or Slave */
	mcbsptest_info.rx_cnt   = 0;
	mcbsptest_info.tx_cnt   = 0;
	mcbsptest_info.num_of_tx_transfers = 256;
	mcbsptest_info.num_of_rx_transfers = 256;
	mcbsptest_info.send_cnt   = 0;
	mcbsptest_info.recv_cnt   = 0;
}

static
int __init omap_mcbsp_init(void)
{

	fill_global_structure();

	/* Requesting interface */
	if(omap2_mcbsp_request_interface(mcbsptest_info.mcbsp_id,OMAP2_MCBSP_MASTER,
							OMAP2_MCBSP_FCLKSRC_PRCM) != 0){
		printk(KERN_ERR "McBSP Test Driver: Requesting mcbsp interface failed\n");
		return -1;
	}

	configure_mcbsp_interface();
	create_proc_file_entries();
	printk("\n OMAP2 McBSP TEST driver installed successfully \n");
	return 0;
}

static
void __exit omap_mcbsp_exit(void)
{
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,00))
	consistent_free((void *)mcbsptest_info.rx_buf_dma_virt, buffer_size, mcbsptest_info.rx_buf_dma_phys);
	consistent_free((void *)mcbsptest_info.tx_buf_dma_virt, buffer_size, mcbsptest_info.tx_buf_dma_phys);
#else
	dma_free_coherent(NULL,buffer_size,(void *)mcbsptest_info.rx_buf_dma_virt, mcbsptest_info.rx_buf_dma_phys);
	dma_free_coherent(NULL,buffer_size,(void *)mcbsptest_info.tx_buf_dma_virt, mcbsptest_info.tx_buf_dma_phys);

#endif

	if(omap2_mcbsp_release_interface(mcbsptest_info.mcbsp_id) != 0){
		printk(KERN_ERR "McBSP Test Driver: Releasing mcbsp interface failed\n");
	}
	remove_proc_file_entries();
	return ;
}

module_init(omap_mcbsp_init);
module_exit(omap_mcbsp_exit);

MODULE_AUTHOR("Texas Instruments");
MODULE_DESCRIPTION("McBSP Test Driver");
MODULE_LICENSE("GPL");

