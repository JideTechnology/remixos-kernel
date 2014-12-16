#include <linux/ion.h>	//for all "ion api"
#include <linux/ion_sunxi.h>	//for import global variable "sunxi_ion_client_create"
#include <linux/dma-mapping.h>	//just include"PAGE_SIZE" macro


/*your data set may look like this*/
static struct 
{
	struct ion_client *client;
	struct ion_handle *handle;
	ion_phys_addr_t phyical_address;
	void* virtual_address;
	size_t address_length;
} your_param ;

/* 
do alloc  , 
	you alloc ion  memory function may looks like it
*/
static int your_module_alloc(int len)
{
	int ret;
//here, create a client for your module.
	your_param.client = sunxi_ion_client_create("define-your-module-name");
	if (IS_ERR(your_param.client))
	{
		pr_err("ion_client_create failed!! %s %s %d" , __FILE__ , __func__ , __LINE__ );
		goto err_client;
	}
	
//here ,you chose CMA heap , no need to cache.
	your_param.handle = ion_alloc( your_param.client, len, PAGE_SIZE,  ION_HEAP_TYPE_DMA_MASK , 0 );
	if (IS_ERR(your_param.handle))
	{
		pr_err("ion_alloc failed!! %s %s %d" , __FILE__ , __func__ , __LINE__ );
		goto err_alloc;
	}	 
//here, we map to virtual address for kernel.
	your_param.virtual_address = ion_map_kernel( your_param.client, your_param.handle);
	if (IS_ERR(your_param.virtual_address))
	{
		pr_err("ion_map_kernel failed!! %s %s %d" , __FILE__ , __func__ , __LINE__ );
		goto err_map_kernel;
	}
//here , we get the phyical address for special usage.
	ret = ion_phys( your_param.client,  your_param.handle, &your_param.phyical_address , &your_param.address_length );
	if( ret )
	{
		pr_err("ion_phys failed!! %s %s %d" , __FILE__ , __func__ , __LINE__ );
		goto err_phys;
	}
	
	return 0;

//got to err process
err_phys:	
	ion_unmap_kernel( your_param.client,  your_param.handle);
err_map_kernel:
	ion_free( your_param.client , your_param.handle );
err_alloc:
	ion_client_destroy(your_param.client);
err_client:

	return -1;

}

/* 
do free  , 
you free ion  memory function may looks like it
*/
static void your_module_free(void)
{
//check 
	if ( IS_ERR(your_param.client) || IS_ERR(your_param.handle) || IS_ERR(your_param.virtual_address))
		return ;
//do reverse functions
	ion_unmap_kernel( your_param.client,  your_param.handle);
	ion_free( your_param.client , your_param.handle );
	ion_client_destroy(your_param.client);

//clear your state here
	your_param.client = (struct ion_client *)0;
	your_param.handle = (struct ion_handle *)0;
	your_param.phyical_address = 0;
	your_param.virtual_address = (void*)0;
	your_param.address_length = 0;
}


/*******************************test module********************************************/
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>


#define M(x)	(x<<20)
static int test_len = M(4);
module_param( test_len , int,0644);

static int __init ion_use_demo_init(void)
{
	return your_module_alloc(test_len);
}

static void __exit ion_use_demo_exit(void)
{
	return your_module_free();
}

module_init(ion_use_demo_init);
module_exit(ion_use_demo_exit);