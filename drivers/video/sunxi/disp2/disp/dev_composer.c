/*-------------------------------------------------------------------------
    
-------------------------------------------------------------------------*/
#ifndef DEV_COMPOSER_C_C
#define DEV_COMPOSER_C_C

#include <linux/sw_sync.h>
#include <linux/sync.h>
#include <linux/file.h>
#include "dev_disp.h"
#include <video/sunxi_display2.h>
//#include <linux/sunxi_tr.h>
#include <linux/slab.h>

static struct   mutex	gcommit_mutek;
static struct   mutex	gwb_mutek;
#if !defined(ALIGN)
#define ALIGN(x,a)	(((x) + (a) - 1L) & ~((a) - 1L))
#endif
#define DBG_TIME_TYPE 3
#define DBG_TIME_SIZE 100
//#define DEBUG_WB
//#define HW_ROTATE_MOD

//#define HW_FOR_HWC

//extern struct sync_pt *sw_sync_pt_create(struct sw_sync_timeline *obj, u32 value);
//extern void sw_sync_timeline_inc(struct sw_sync_timeline *obj, u32 inc);
//extern int sync_fence_wait(struct sync_fence *fence, long timeout);
//extern struct sw_sync_timeline *sw_sync_timeline_create(const char *name);


#define DISP_NUMS_SCREEN 2
enum {
    /* flip source image horizontally (around the vertical axis) */
    HAL_TRANSFORM_FLIP_H    = 0x01,
    /* flip source image vertically (around the horizontal axis)*/
    HAL_TRANSFORM_FLIP_V    = 0x02,
    /* rotate source image 90 degrees clockwise */
    HAL_TRANSFORM_ROT_90    = 0x04,
    /* rotate source image 180 degrees */
    HAL_TRANSFORM_ROT_180   = 0x03,
    /* rotate source image 270 degrees clockwise */
    HAL_TRANSFORM_ROT_270   = 0x07,
    /* don't use. see system/window.h */
    HAL_TRANSFORM_RESERVED  = 0x08,
};

enum {
    HWC_DISPLAY_NOACTIVE = 0,
    HWC_DISPLAY_PREACTIVE,
    HWC_DISPLAY_ACTIVE,
};

struct composer_health_info
{
	unsigned long         time[DBG_TIME_TYPE][DBG_TIME_SIZE];
	unsigned int          time_index[DBG_TIME_TYPE];
	unsigned int          count[DBG_TIME_TYPE];
};

struct WriteBack{
    int                         syncfd;
    int                         rotate;
    struct disp_capture_info    capturedata;
};

struct setup_dispc_data
{
    int                         forceflip;
    int                         layer_num[2];
    struct disp_layer_config    layer_info[2][16];
    int                         firstdisplay;
    int                         *aquireFenceFd;
    int                         aquireFenceCnt;
    int                         firstDispFenceCnt;
    int                         *returnfenceFd;
    bool                        needWB[2]; //[0] is HDMI, [1] is miracast
    unsigned int                ehancemode[2]; //0 is close,1 is whole,2 is half mode
    unsigned int                androidfrmnum;
    struct WriteBack            *WriteBackdata;
};

struct format_info
{
    enum disp_pixel_format      format;
    unsigned char               bpp;
    unsigned char               plan;
    unsigned char               plnbpp[3];
    unsigned char               plan_scale_w[3];
    unsigned char               plan_scale_h[3];
    unsigned char               align[3];
    bool                        swapUV;
};

enum HWC_IOCTL
{
    HWC_IOCTL_COMMIT = 0,
    HWC_IOCTL_WBSYNC = 1,
    HWC_IOCTL_ALLIGN = 2,
};

struct hwc_ioctl_arg
{
    enum HWC_IOCTL  cmd;
    void            *arg;
};

struct hwc_sync_info
{
    unsigned int    timelinenum;
    unsigned int    androidOrWB;
    unsigned int    vsyncVeryfy;
};

struct WB_data_list
{
    struct list_head            list;
    bool                        isforHDMI;
    struct hwc_sync_info        *psSyncInfo;
    struct sync_fence           *outaquirefence;
    struct disp_capture_info    *pscapture;
    struct WriteBack            WriteBackdata;
};

struct dispc_data_list
{
    struct list_head            list;
    struct hwc_sync_info        *psSyncInfo;
    struct WB_data_list         *WB_data;
    struct setup_dispc_data     hwc_data;
    struct sync_fence           **fence_array;;
};

struct dumplayer
{
    struct list_head    list;
    void                *vm_addr;
    void                *p_addr;
    unsigned int        size;
    unsigned int        androidfrmnum; 
    bool                isphaddr;
    bool                update;
};

struct wb_layer
{
    void                    *p_addr;
    unsigned int            size;
    struct disp_rectsz      screen;
    struct disp_rect        crop;
    unsigned int            frame;
    enum disp_pixel_format  format;
    bool                    isfree;
    bool                    islayer;
};

struct miracast_handle
{
    struct list_head    list;
    struct wb_layer     handle_info;
};

enum FREE_QUEUE
{
    FREE_LITTLE,
    FREE_PREC,
    FREE_ALL
};

struct composer_private_data
{
	struct work_struct          post2_cb_work;
	unsigned int	            Cur_Write_Cnt;
    unsigned int                Cur_Disp2_WB;
	unsigned int                Cur_Disp_Cnt[2];
    unsigned int                last_wb_cnt;
    unsigned int                Cur_Disp_wb;
	bool                        b_no_output;
    bool                        countrotate[2];
    char                        display_active[2];
	struct mutex	            runtime_lock;
	struct list_head            update_regs_list;

	unsigned int                timeline_max;
	struct mutex                update_regs_list_lock;
	spinlock_t                  update_reg_lock;
	struct work_struct          commit_work;
    struct workqueue_struct     *Display_commit_work;
    struct sw_sync_timeline     *relseastimeline;

	struct composer_health_info health_info;
    int                         tr_fd;
    struct setup_dispc_data     *tmptransfer;
    disp_drv_info               *psg_disp_drv;
    unsigned int                ehancemode[2];
    
    struct list_head            dumplyr_list;
    spinlock_t                  dumplyr_lock;
    unsigned int                listcnt;
    unsigned char               dumpCnt;
    unsigned char               display;
    unsigned char               layerNum;
    unsigned char               channelNum;
    unsigned char               firstdisp;
    int                         controlbywb:1;
    int                         pause:1;
    int                         cancel:1;
    int                         dumpstart:1;
    int                         initrial:1;

    struct work_struct          WB_work;
    struct workqueue_struct     *Display_WB_work;
    struct sw_sync_timeline     *writebacktimeline;
    struct list_head            WB_list;//current only display0
    struct list_head            WB_err_list;
    struct list_head            WB_miracast_list;
    unsigned int                WB_count;
    spinlock_t                  WB_lock;
    struct mutex	            queue_lock;
    bool                        WB_status[2];
    bool                        need_rotate;
    struct wb_layer             *wb_queue;
    int                         queue_num;
    int                         rotate_num;
    int                         max_queue;
    struct disp_layer_config    *layer_info;
    struct wb_layer             *pswb_now;
    wait_queue_head_t           waite_for_vsync;;
    unsigned int                vsyncnum;

    struct kmem_cache           *disp_slab;
    char                        hw_allined;
    char                        yuv_alligned;

};

static struct composer_private_data composer_priv;

#if defined(HW_ROTATE_MOD)
extern int sunxi_tr_request(void);
extern int sunxi_tr_release(int hdl);
extern int sunxi_tr_commit(int hdl, struct tr_info *info);
extern int sunxi_tr_query(int hdl);
#endif

static void imp_finish_cb(bool force_all);

static inline bool hwc_compare_crop(struct disp_rect *src_crop, struct disp_rect  *dst_crop)
{
    if(src_crop->x != dst_crop->x || src_crop->y != dst_crop->y 
                        ||src_crop->width != dst_crop->width ||src_crop->height != dst_crop->height )
    {
        return 1;
    }
    return 0;
}

static inline bool hwc_compare_screen(struct disp_rectsz *src_screen,struct disp_rectsz *dst_screen)
{
   if( src_screen->width == dst_screen->width && src_screen->height == dst_screen->height )
   {
        return 0;
   }
   return 1;
}

static bool inline hwc_format_info(struct format_info *format_info,enum disp_pixel_format format)
{
    format_info->format = format;
    format_info->align[0] = composer_priv.hw_allined;
    format_info->plan = 1;
    format_info->plan_scale_h[0] = 1;
    format_info->plan_scale_w[0] = 1;
    format_info->swapUV = 0;
    format_info->bpp = 32;
    format_info->plnbpp[0] = 32;
    switch(format_info->format)
    {
        case DISP_FORMAT_ABGR_8888:
        case DISP_FORMAT_ARGB_8888:
        case DISP_FORMAT_XRGB_8888:
        case DISP_FORMAT_XBGR_8888:
            format_info->bpp = 32;
            format_info->plan = 1;
            format_info->plnbpp[0] = 32;
            format_info->plnbpp[1] = 0;
            format_info->plnbpp[2] = 0;
        break;
        case DISP_FORMAT_BGR_888:
            format_info->bpp = 24;
            format_info->plan = 1;
            format_info->plnbpp[0] = 24;
            format_info->plnbpp[1] = 0;
            format_info->plnbpp[2] = 0;
        break;
        case DISP_FORMAT_RGB_565:
            format_info->bpp = 16;
            format_info->plan = 1;
            format_info->plnbpp[0] = 16;
            format_info->plnbpp[1] = 0;
            format_info->plnbpp[2] = 0;
        break;
        case DISP_FORMAT_YUV420_P:
            format_info->bpp = 12;
            format_info->plan = 3;
            format_info->plnbpp[0] = 8;
            format_info->plnbpp[1] = 8;
            format_info->plnbpp[2] = 8;
            format_info->align[0] = composer_priv.yuv_alligned;
            format_info->align[1] = composer_priv.yuv_alligned/2;
            format_info->align[2] = composer_priv.yuv_alligned/2;
            format_info->plan_scale_h[1] = 2;
            format_info->plan_scale_w[1] = 2;
            format_info->plan_scale_h[2] = 2;
            format_info->plan_scale_w[2] = 2;
            format_info->swapUV = 1;
        break;
        case DISP_FORMAT_YUV420_SP_VUVU:
        case DISP_FORMAT_YUV420_SP_UVUV:
            format_info->bpp = 12;
            format_info->plan = 2;
            format_info->plnbpp[0] = 8;
            format_info->plnbpp[1] = 16;
            format_info->plnbpp[2] = 0;
            format_info->align[0] = composer_priv.yuv_alligned;
            format_info->align[1] = composer_priv.yuv_alligned/2;
            format_info->plan_scale_h[1] = 2;
            format_info->plan_scale_w[1] = 2;

        break;
        default:
            printk("we got a err info..\n");
            return 1;
   }
    return 0;
}
#if defined(HW_FOR_HWC)
bool hwc_memset(struct wb_layer *pswb_queue,struct format_info *format_info)
{
    void *vaddr = NULL,*iniaddr = NULL;
    int i = 0, j = 0;
    unsigned int size0 = 0, size1 = 0, size2 = 0;
    vaddr = sunxi_map_kernel((unsigned int)pswb_queue->p_addr,pswb_queue->size);
    if(vaddr == NULL)
    {
        return 1;
    }
    iniaddr = vaddr;
    while(i < format_info->plan)
    {
        size0 = format_info->plnbpp[i] * pswb_queue->crop.y /format_info->plan_scale_h[i] * pswb_queue->screen.width / format_info->plan_scale_w[i] /8;
        memset(vaddr, 0, size0);
        size1 = format_info->plnbpp[i] * (pswb_queue->crop.y + pswb_queue->crop.height) / format_info->plan_scale_h[i] * pswb_queue->screen.width / format_info->plan_scale_w[i] /8;
        size2 = format_info->plnbpp[i] * (pswb_queue->screen.height - pswb_queue->crop.y - pswb_queue->crop.height) / format_info->plan_scale_h[i] * pswb_queue->screen.width / format_info->plan_scale_w[i] /8;
        memset(vaddr + size1, 0, size2);
        vaddr += size0;
        size0 = format_info->plnbpp[i] * (pswb_queue->crop.x / format_info->plan_scale_w[i]) /8;
        size1 = format_info->plnbpp[i] * (pswb_queue->screen.width - pswb_queue->crop.x - pswb_queue->crop.width) / format_info->plan_scale_w[i] /8;
        j = 0;
        while( j < pswb_queue->crop.height / format_info->plan_scale_h[i])
        {
            memset(vaddr, 0, size0);
            memset(vaddr + format_info->plnbpp[i] *(pswb_queue->crop.x + pswb_queue->crop.width) / format_info->plan_scale_w[i] /8, 0, size1);
            vaddr += format_info->plnbpp[i] * (pswb_queue->screen.width / format_info->plan_scale_w[i]) /8;
            j++;
        }
        vaddr += size2;
        i++;
    }
    sunxi_unmap_kernel(iniaddr);
    return 0;
}

struct wb_layer * wb_dequeue(struct disp_rectsz screen, struct disp_rect crop, enum disp_pixel_format format, unsigned int frame,bool islayer)
{
    int i = 0,fix = 255;
    struct wb_layer  *pswb_queue = NULL;
    struct format_info format_info;
    int allocsize;
//here  only the hwc_setup_wbdata() function setup, no need mutex 
    if(composer_priv.wb_queue == NULL)
    {
        composer_priv.wb_queue = kzalloc(sizeof(wb_layer)*composer_priv.max_queue,GFP_KERNEL);
        if(composer_priv.wb_queue == NULL)
        {
            printk("alloc the wb_queue err.\n");
            goto err;
        }
        composer_priv.queue_num = 0;
    }
    if(hwc_format_info(&format_info,format))
    {
        printk("get format err\n");
        goto err;
    }
    allocsize = ALIGN(screen.width,format_info.align[0]) * screen.height * format_info.bpp /8;

    //here add a mutex
    mutex_lock(&composer_priv.queue_lock);
    while( i < composer_priv.queue_num )
    {
        pswb_queue = &composer_priv.wb_queue[i];
        if(pswb_queue->p_addr != NULL)
        {
            if(pswb_queue->isfree)
            {
                if(!hwc_compare_screen(&pswb_queue->screen,&screen) && pswb_queue->format == format)
                {
                    if(hwc_compare_crop(&pswb_queue->crop,&crop))
                    {
                        pswb_queue->crop = crop;
                        if(hwc_memset(pswb_queue,&format_info))
                        {
                            mutex_unlock(&composer_priv.queue_lock);
                            goto err;
                        }
                    }
                    goto ret_ok;
                }else{
                    sunxi_mem_free((unsigned int)pswb_queue->p_addr,pswb_queue->size);
                    pswb_queue->p_addr = NULL;
                    pswb_queue->size = 0;
                    pswb_queue->frame = 0;
                    pswb_queue->screen.width = 0;
                    pswb_queue->screen.height = 0;
                    fix = i;
                }
            }
        }else{
            fix = i;
            break;
        }
        i++;
    }
    if(fix == 255 && composer_priv.queue_num < composer_priv.max_queue)
    {
        fix = composer_priv.queue_num;
        composer_priv.queue_num++;
    }
    if(fix == 255)
    {
        printk("hwc wb have no buffer:wbsend[%d][%d][%d][%d]\n",composer_priv.Cur_Disp_Cnt[0],composer_priv.last_wb_cnt,composer_priv.Cur_Disp2_WB,composer_priv.Cur_Disp_wb);
        mutex_unlock(&composer_priv.queue_lock);
        goto err;
    }
    pswb_queue = &composer_priv.wb_queue[fix];
    pswb_queue->p_addr = (void *)sunxi_mem_alloc(allocsize);
    if(pswb_queue->p_addr == NULL)
    {
        mutex_unlock(&composer_priv.queue_lock);
        goto err;
    }
    pswb_queue->screen = screen;
    pswb_queue->crop = crop;
    pswb_queue->size = allocsize;
    pswb_queue->format = format;
    if(hwc_memset(pswb_queue,&format_info))
    {
        mutex_unlock(&composer_priv.queue_lock);
        goto err;
    }
ret_ok:
    pswb_queue->frame = frame;
    pswb_queue->isfree = 0;
    pswb_queue->islayer = islayer;
    mutex_unlock(&composer_priv.queue_lock);
    return pswb_queue;

err:
    return NULL;
}

static inline void wb_queue(unsigned int frame,enum FREE_QUEUE precision,bool islayer)
{
    struct wb_layer *pswb_queue = NULL;
    int i = 0;
    while(i < composer_priv.queue_num)
    {
        pswb_queue = &composer_priv.wb_queue[i];
        if( (precision == FREE_LITTLE && pswb_queue->frame < frame)
            ||(precision == FREE_PREC && pswb_queue->frame == frame && pswb_queue->islayer)
            ||(precision == FREE_ALL)
          )
        {
            pswb_queue->isfree = 1;
        }
        i++;
    }
}

void wb_free(void)
{
    int i = 0;
    struct wb_layer  *pswb_queue = NULL;
    if(composer_priv.wb_queue)
    {
        while(i < composer_priv.queue_num)
        {
            pswb_queue = &composer_priv.wb_queue[i];
            if(pswb_queue->p_addr != NULL)
            {
                sunxi_mem_free((unsigned int)pswb_queue->p_addr,pswb_queue->size);
                pswb_queue->p_addr = NULL;
                pswb_queue->size = 0;
                pswb_queue->frame = 0;
            }
            i++;
        }
        kfree(composer_priv.wb_queue);
    }
    printk("hwc  wb_free the memory\n");
    composer_priv.queue_num = 0;
    composer_priv.wb_queue = NULL;
}

void hwc_setup_layer(struct disp_layer_config *pslayer_info,struct  disp_s_frame *out_frame,struct  disp_rectsz *screen, unsigned long long addr)
{
    int i = 0;
    struct format_info format_info;

    pslayer_info->channel = 0;
    pslayer_info->enable = 1;
    pslayer_info->layer_id = 0;
    pslayer_info->info.zorder = 0;
    pslayer_info->info.screen_win = out_frame->crop;
    pslayer_info->info.fb.crop.x = 0;
    pslayer_info->info.fb.crop.y = 0;
    pslayer_info->info.fb.crop.width = ((long long)screen->width)<<32;
    pslayer_info->info.fb.crop.height = ((long long)screen->height)<<32;
    pslayer_info->info.fb.format = out_frame->format;
    hwc_format_info(&format_info,out_frame->format);
    pslayer_info->info.fb.addr[0] = addr;
    while(i < format_info.plan)
    {
        if(i>0)
        {
            pslayer_info->info.fb.addr[i] = pslayer_info->info.fb.addr[i-1]
                +pslayer_info->info.fb.size[i-1].width * pslayer_info->info.fb.size[i-1].height * format_info.plnbpp[i-1] / 8;
        }
        pslayer_info->info.fb.size[i].width = screen->width / format_info.plan_scale_w[i];
        pslayer_info->info.fb.size[i].height = screen->height / format_info.plan_scale_h[i];
        pslayer_info->info.fb.align[i] = format_info.align[i];
        i++;
    }
    if(format_info.swapUV)
    {
        unsigned long long swapAddr;
        swapAddr = pslayer_info->info.fb.addr[1];
        pslayer_info->info.fb.addr[1] = pslayer_info->info.fb.addr[2];
        pslayer_info->info.fb.addr[2] = swapAddr;
    }
    i = 1;
    while(i < 8)
    {
        pslayer_info++;
        pslayer_info->channel = i/4;
        pslayer_info->enable = 0;
        pslayer_info->layer_id = i%4;
        i++;
    }
}

static inline unsigned int  hwc_get_sync( void)
{
    return composer_priv.Cur_Disp_wb;
}
#endif
int dispc_gralloc_queue(struct setup_dispc_data *psDispcData, struct hwc_sync_info *sync, struct disp_capture_info *psWBdata);

//type: 0:acquire, 1:release; 2:display
static s32 composer_get_frame_fps(unsigned int type)
{
	unsigned int pre_time_index, cur_time_index;
	unsigned int pre_time, cur_time;
	unsigned int fps = 0xff;

	pre_time_index = composer_priv.health_info.time_index[type];
	cur_time_index = (pre_time_index == 0)? (DBG_TIME_SIZE -1):(pre_time_index-1);

	pre_time = composer_priv.health_info.time[type][pre_time_index];
	cur_time = composer_priv.health_info.time[type][cur_time_index];

	if(pre_time != cur_time) {
		fps = 1000 * 100 / (cur_time - pre_time);
	}

	return fps;
}

//type: 0:acquire, 1:release; 2:display
static void composer_frame_checkin(unsigned int type)
{
	unsigned int index = composer_priv.health_info.time_index[type];
	composer_priv.health_info.time[type][index] = jiffies;
	index ++;
	index = (index>=DBG_TIME_SIZE)?0:index;
	composer_priv.health_info.time_index[type] = index;
	composer_priv.health_info.count[type]++;
}

unsigned int composer_dump(char* buf)
{
	unsigned int fps0,fps1,fps2;
	unsigned int cnt0,cnt1,cnt2;

	fps0 = composer_get_frame_fps(0);
	fps1 = composer_get_frame_fps(1);
	fps2 = composer_get_frame_fps(2);
	cnt0 = composer_priv.health_info.count[0];
	cnt1 = composer_priv.health_info.count[1];
	cnt2 = composer_priv.health_info.count[2];

	return sprintf(buf, "acquire: %d, %d.%d fps\nrelease: %d, %d.%d fps\ndisplay: %d, %d.%d fps\n",
		cnt0, fps0/10, fps0%10, cnt1, fps1/10, fps1%10, cnt2, fps2/10, fps2%10);
}
#if defined(HW_FOR_HWC)
int dev_composer_debug(setup_dispc_data_t *psDispcData, unsigned int framenuber)
{
    unsigned int  size = 0 ;
    void *kmaddr = NULL,*vm_addr = NULL,*sunxi_addr = NULL;
    struct disp_layer_config *dumlayer = NULL;
    struct dumplayer *dmplyr = NULL, *next = NULL, *tmplyr = NULL;
    bool    find = 0;
    struct format_info format_info;
    if(composer_priv.cancel && composer_priv.display != 255 && composer_priv.channelNum != 255 && composer_priv.layerNum != 255)
    {
        dumlayer = &(psDispcData->layer_info[composer_priv.display][composer_priv.channelNum*4+composer_priv.layerNum]);
        dumlayer->enable =0;
    }
    if(composer_priv.dumpstart)
    {
        if(composer_priv.dumpCnt > 0 && composer_priv.display != 255 && composer_priv.channelNum != 255 && composer_priv.layerNum != 255 )
        {
            dumlayer = &(psDispcData->layer_info[composer_priv.display][composer_priv.channelNum*4 + composer_priv.layerNum]);
            if(dumlayer != NULL && dumlayer->enable)
            {
                if(hwc_format_info(&format_info,dumlayer->info.fb.format))
                {
                    goto err;
                }
                size = dumlayer->info.fb.size[0].width * dumlayer->info.fb.size[0].height * format_info.bpp / 8;
                sunxi_addr = sunxi_map_kernel((unsigned int)dumlayer->info.fb.addr[0],size);
                if(sunxi_addr == NULL)
                {
                    printk("map mem err...\n");
                    goto err;
                }
                list_for_each_entry_safe(dmplyr, next, &composer_priv.dumplyr_list, list)
                {
                    if(!dmplyr->update)
                    {
                        if(dmplyr->size == size)
                        {
                            find = 1;
                            break;
                        }else{
                            tmplyr = dmplyr;
                        }
                    }
                }
                if(find)
                {
                    memcpy(dmplyr->vm_addr, sunxi_addr, size);
                    sunxi_unmap_kernel(sunxi_addr);
                    dmplyr->update = 1;
                    dmplyr->androidfrmnum = framenuber;
                    composer_priv.dumpCnt--;
                    goto ok;
                }else if(tmplyr != NULL)
                {
                    dmplyr = tmplyr;
                     if(dmplyr->isphaddr)
                        {
                            sunxi_unmap_kernel(dmplyr->vm_addr);
                            sunxi_mem_free((unsigned int)dmplyr->p_addr,dmplyr->size);
                        }else{
                            kfree((void *)dmplyr->vm_addr);
                        }
                        dmplyr->vm_addr = NULL;
                        dmplyr->p_addr = NULL;
                        dmplyr->size = 0;
                }else{
                    dmplyr = kzalloc(sizeof(dumplayer_t),GFP_KERNEL);
                    if(dmplyr == NULL)
                    {
                        printk("kzalloc mem err...\n");
                        sunxi_unmap_kernel(sunxi_addr);
                        goto err;
                    }
                }
                kmaddr = (void *)sunxi_mem_alloc(size);
                dmplyr->isphaddr = 1;
                if(kmaddr == 0)
                {
                    vm_addr = kzalloc(size,GFP_KERNEL);
                    dmplyr->isphaddr = 0;
                    if(vm_addr == NULL)
                    {
                        sunxi_unmap_kernel(sunxi_addr);
                        dmplyr->update = 0;
                        printk("kzalloc mem err...\n");
                        goto err;
                    }
                    dmplyr->vm_addr = vm_addr;
                }
                if(dmplyr->isphaddr)
                {
                    dmplyr->vm_addr = sunxi_map_kernel((unsigned int)kmaddr,size);
                    if(dmplyr->vm_addr == NULL)
                    {
                        sunxi_unmap_kernel(sunxi_addr);
                        dmplyr->update = 0;
                        printk("kzalloc mem err...\n ");
                        goto err;
                    }
                    dmplyr->p_addr = kmaddr;
                }
                dmplyr->size = size;
                dmplyr->androidfrmnum = framenuber;
                memcpy(dmplyr->vm_addr, sunxi_addr, size);
                sunxi_unmap_kernel(sunxi_addr);
                dmplyr->update = 1;
                if(tmplyr == NULL)
                {
                    composer_priv.listcnt++;
                    list_add_tail(&dmplyr->list, &composer_priv.dumplyr_list);
                }
            }
            composer_priv.dumpCnt--;
        }
    }
ok:
    return 0;
err:
    return -1;
}

static int debug_write_file(struct dumplayer *dumplyr)
{
    char s[30];
    struct file *dumfile;
    mm_segment_t old_fs;
    int cnt;
    if(dumplyr->vm_addr != NULL && dumplyr->size != 0)
    {
        cnt = sprintf(s, "/mnt/sdcard/dumplayer%d",dumplyr->androidfrmnum);
        dumfile = filp_open(s, O_RDWR|O_CREAT, 0755);
        if(IS_ERR(dumfile))
        {
            printk("open %s err[%d]\n",s,(int)dumfile);
            return 0;
        }
        old_fs = get_fs();
        set_fs(KERNEL_DS);
        if(dumplyr->vm_addr != NULL && dumplyr->size !=0)
        {
            dumfile->f_op->write(dumfile, dumplyr->vm_addr, dumplyr->size, &dumfile->f_pos);
        }
        set_fs(old_fs);
        filp_close(dumfile, NULL);
        dumfile = NULL;
        dumplyr->update = 0;
    }
   return 0 ;
}

static bool hwc_check_needwq(struct hwc_sync_info *psSyncInfo, struct wb_layer *psNow,struct disp_rectsz *screen, struct disp_rect *crop)
{
    if(psSyncInfo->timelinenum - composer_priv.Cur_Disp_Cnt[0] <= 1)
    {
        goto need;
    }
    if(psNow == NULL)
    {
        goto need;
    }
    if(hwc_compare_crop(&psNow->crop,crop) || hwc_compare_screen(&psNow->screen,screen))
    {
        goto need;
    }
    return 0;
need:
    return 1;
}

static bool hwc_check_miracat(struct disp_s_frame *psout_frame)
{
    struct miracast_handle  *data = NULL, *next = NULL;
    struct wb_layer *pswb_layer = NULL;
    bool find = 0;
    struct format_info format_info;
    list_for_each_entry_safe(data, next, &composer_priv.WB_miracast_list, list)
    {
        if(((unsigned int)data->handle_info.p_addr) == psout_frame->addr[0])
        {
            find = 1;
            break;
        }
    }
    if(!find)
    {
        data = kzalloc(sizeof(miracast_handle_t),GFP_KERNEL);
        if(data == NULL)
        {
            goto ret_err;
        }
        list_add_tail(&data->list, &composer_priv.WB_miracast_list);
    }
    pswb_layer = &data->handle_info;
    if(pswb_layer->format == psout_frame->format
        && !hwc_compare_crop(&pswb_layer->crop,&psout_frame->crop)
        && hwc_compare_screen(&pswb_layer->screen, &psout_frame->size[0]))
    {
        goto ret_ok;
    }
    hwc_format_info(&format_info,psout_frame->format);
    pswb_layer->format = psout_frame->format;
    pswb_layer->crop = psout_frame->crop;
    pswb_layer->screen = psout_frame->size[0];
    hwc_memset(pswb_layer,&format_info);

ret_ok:
    return 0;
ret_err:
    return 1;
}

static inline void hwc_setup_capture(struct disp_s_frame *psout_frame,struct format_info *format_info,struct disp_rectsz screen,struct disp_rect crop,struct wb_layer *pswb_now)
{
    int i = 1;
    psout_frame->addr[0] = (unsigned int)pswb_now->p_addr;
    psout_frame->crop = crop;
    psout_frame->size[0] = screen;
    psout_frame->format = format_info->format;

    while(i < format_info->plan)
    {
        psout_frame->size[i].width = screen.width / format_info->plan_scale_w[i];
        psout_frame->size[i].height = screen.height / format_info->plan_scale_h[i];
        psout_frame->addr[i] = psout_frame->addr[i-1] + psout_frame->size[i-1].width * psout_frame->size[i-1].height * format_info->plnbpp[i-1] /8;
        i++;
    }
    if(format_info->swapUV)
    {
        unsigned long long swapAddr;
        swapAddr = psout_frame->addr[1];
        psout_frame->addr[1] = psout_frame->addr[2];
        psout_frame->addr[2] = swapAddr;
    }
}

static struct disp_capture_info * hwc_setup_wbdata(struct WB_data_list *WB_data, struct hwc_sync_info *psSyncInfo)
{
    struct disp_s_frame *psout_frame = NULL;
    struct disp_rect *pswindow = NULL;
    bool  haserr = 0;
    struct disp_rectsz screen;
    struct disp_rect crop;
    struct format_info format_info;
    enum disp_pixel_format format;
    
    if(WB_data != NULL)
    {
#if defined(HW_ROTATE_MOD)
        if(composer_priv.tr_fd  == 0)
        {
            composer_priv.tr_fd = sunxi_tr_request();
            if(composer_priv.tr_fd == (int)NULL)
            {
                haserr = 1;
                printk("%s   get tr_fd failed\n",__func__);
                goto ret_wb;
            }
        }
#endif
        composer_priv.need_rotate = !!WB_data->WriteBackdata.rotate;
        format = WB_data->WriteBackdata.capturedata.out_frame.format;
        hwc_format_info(&format_info,format);
        psout_frame = &WB_data->WriteBackdata.capturedata.out_frame;
        pswindow = &WB_data->WriteBackdata.capturedata.window;
        if(WB_data->isforHDMI)
        {
            screen.width = pswindow->width;
            screen.height = pswindow->height;
            crop.x = pswindow->x;
            crop.y = pswindow->y;
            crop.width = pswindow->width;
            crop.height = pswindow->height;
            composer_priv.controlbywb = WB_data->isforHDMI;
        }else{
            if(WB_data->WriteBackdata.rotate & HAL_TRANSFORM_ROT_90)
            {
                screen.width = psout_frame->size[0].height;
                screen.height = psout_frame->size[0].width;
                crop.x = psout_frame->crop.y;
                crop.y = psout_frame->crop.x;
                crop.width = psout_frame->crop.height;
                crop.height = psout_frame->crop.width;
            }else{
                screen.width = psout_frame->size[0].width;
                screen.height = psout_frame->size[0].height;
                crop = psout_frame->crop;
            }
        }
        if(WB_data->WriteBackdata.rotate
            || ( WB_data->outaquirefence != NULL ? !WB_data->outaquirefence->status : 0)
            || WB_data->isforHDMI)
        {
            WB_data->pscapture = kzalloc(sizeof(disp_capture_info), GFP_KERNEL);
            if(WB_data->pscapture == NULL)
            {
                haserr = 1;
                printk("%s   kzalloc err\n",__func__);
                goto ret_wb;
            }
            WB_data->pscapture->window = WB_data->WriteBackdata.capturedata.window;
            psout_frame = &WB_data->pscapture->out_frame;
            if(hwc_check_needwq(psSyncInfo, composer_priv.pswb_now,&screen,&crop))
            {
                composer_priv.pswb_now = wb_dequeue(screen, crop, format, psSyncInfo->timelinenum,0);
                if( composer_priv.pswb_now == NULL )
                {
                    haserr = 1;
                    goto ret_wb;
                }
            }else{
                if(psSyncInfo->vsyncVeryfy == psSyncInfo->timelinenum)
                {
                    psSyncInfo->vsyncVeryfy = composer_priv.Cur_Disp_Cnt[0];
                }
            }

            hwc_setup_capture(psout_frame,&format_info,screen,crop,composer_priv.pswb_now);
        }else{
            if(hwc_check_miracat(psout_frame))
            {
                haserr = 1;
                goto ret_haserr;
            }
        }
ret_wb:
        spin_lock(&composer_priv.WB_lock);
        list_add_tail(&(WB_data->list), &composer_priv.WB_list);
        spin_unlock(&composer_priv.WB_lock);
#if defined(DEBUG_WB)
        printk("\ntime:%d\nLCD[%d,%d,%d,%d] ADDR[0x%llx,0x%llx,0x%llx] Size[[%d,%d][%d,%d][%d,%d]] TR:%02x"
                 "\nVir[%d,%d,%d,%d] ADDR[0x%llx,0x%llx,0x%llx] Size[[%d,%d][%d,%d][%d,%d]] \n\n"
            ,psSyncInfo->timelinenum
            ,WB_data->pscapture != NULL ? WB_data->pscapture->out_frame.crop.x:0
            ,WB_data->pscapture != NULL ? WB_data->pscapture->out_frame.crop.y:0
            ,WB_data->pscapture != NULL ? WB_data->pscapture->out_frame.crop.width:0
            ,WB_data->pscapture != NULL ? WB_data->pscapture->out_frame.crop.height:0
            ,WB_data->pscapture != NULL ? WB_data->pscapture->out_frame.addr[0]:0
            ,WB_data->pscapture != NULL ? WB_data->pscapture->out_frame.addr[1]:0
            ,WB_data->pscapture != NULL ? WB_data->pscapture->out_frame.addr[2]:0
            ,WB_data->pscapture != NULL ? WB_data->pscapture->out_frame.size[0].width:0
            ,WB_data->pscapture != NULL ? WB_data->pscapture->out_frame.size[0].height:0
            ,WB_data->pscapture != NULL ? WB_data->pscapture->out_frame.size[1].width:0
            ,WB_data->pscapture != NULL ? WB_data->pscapture->out_frame.size[1].height:0
            ,WB_data->pscapture != NULL ? WB_data->pscapture->out_frame.size[2].width:0
            ,WB_data->pscapture != NULL ? WB_data->pscapture->out_frame.size[2].height:0
            ,WB_data->pscapture != NULL ? WB_data->WriteBackdata.rotate:0
            ,WB_data->WriteBackdata.capturedata.out_frame.crop.x
            ,WB_data->WriteBackdata.capturedata.out_frame.crop.y
            ,WB_data->WriteBackdata.capturedata.out_frame.crop.width
            ,WB_data->WriteBackdata.capturedata.out_frame.crop.height
            ,WB_data->WriteBackdata.capturedata.out_frame.addr[0]
            ,WB_data->WriteBackdata.capturedata.out_frame.addr[1]
            ,WB_data->WriteBackdata.capturedata.out_frame.addr[2]
            ,WB_data->WriteBackdata.capturedata.out_frame.size[0].width
            ,WB_data->WriteBackdata.capturedata.out_frame.size[0].height
            ,WB_data->WriteBackdata.capturedata.out_frame.size[1].width
            ,WB_data->WriteBackdata.capturedata.out_frame.size[1].height
            ,WB_data->WriteBackdata.capturedata.out_frame.size[2].width
            ,WB_data->WriteBackdata.capturedata.out_frame.size[2].height
            );
#endif
    }else{
#if defined(HW_ROTATE_MOD)    
        if(composer_priv.tr_fd  != 0)
        {
            sunxi_tr_release(composer_priv.tr_fd);
            composer_priv.tr_fd = 0;
        }
#endif
        haserr = 1;
    }
ret_haserr:
    if(haserr)
    {
        if(WB_data != NULL && WB_data->pscapture != NULL)
        {
            kfree(WB_data->pscapture);
            WB_data->pscapture = NULL;
        }
        return NULL;
    }
    return WB_data->pscapture != NULL ? WB_data->pscapture : &WB_data->WriteBackdata.capturedata;
}
#endif
static void inline hwc_check_alive(struct setup_dispc_data *disp_data)
{
    int forwhile = DISP_NUMS_SCREEN;
    bool needresigned = 0;
    if(composer_priv.firstdisp != disp_data->firstdisplay)
    {
        needresigned = 1;
        composer_priv.firstdisp = disp_data->firstdisplay;
    }
    while(forwhile-- && !disp_data->forceflip)
    {
        if(forwhile != disp_data->firstdisplay || needresigned)
        {
            if((composer_priv.display_active[forwhile] == HWC_DISPLAY_NOACTIVE && disp_data->layer_num[forwhile]) || needresigned)
            {
                composer_priv.display_active[forwhile] = HWC_DISPLAY_PREACTIVE;
            }
            if(!disp_data->layer_num[forwhile])
            {
                composer_priv.display_active[forwhile] = HWC_DISPLAY_NOACTIVE;
            }
         }else{
            if(composer_priv.initrial == HWC_DISPLAY_NOACTIVE)
            {
                composer_priv.display_active[forwhile] = HWC_DISPLAY_PREACTIVE;
                composer_priv.initrial = 1;
            }
        }
    }
    if(needresigned)
    {
       unsigned int vysncnum = composer_priv.vsyncnum;
       int err = wait_event_interruptible_timeout(composer_priv.waite_for_vsync,vysncnum != composer_priv.vsyncnum,3000);
       if(err < 0)
       {
           	mutex_lock(&composer_priv.runtime_lock);
	        imp_finish_cb(1);
	        mutex_unlock(&composer_priv.runtime_lock);
       }
    }
}

static void hwc_commit_work(struct work_struct *work)
{
    struct dispc_data_list *data, *next;
    struct list_head saved_list;
    int err,i,disp;

    struct disp_capture_info *psWBdata = NULL;
    struct sync_fence *AcquireFence = NULL;

    mutex_lock(&(gcommit_mutek));
    mutex_lock(&(composer_priv.update_regs_list_lock));
    list_replace_init(&composer_priv.update_regs_list, &saved_list);
    mutex_unlock(&(composer_priv.update_regs_list_lock));

    list_for_each_entry_safe(data, next, &saved_list, list)
    {
        disp = 0;
        list_del(&data->list);
        if(data->hwc_data.forceflip)
        {
            printk("HWC give a forcflip frame\n");
            printk("androidfrmnum:%d  ker:%d  timeline:%d\n",data->hwc_data.androidfrmnum ,data->psSyncInfo->timelinenum,composer_priv.relseastimeline->value);
            composer_priv.Cur_Write_Cnt = data->psSyncInfo->timelinenum;
            sw_sync_timeline_inc(composer_priv.relseastimeline, 1);
            if(data->WB_data != NULL)
            {
                kfree(data->WB_data);
            }
            goto free;
        }
	    for(i = 0; i < data->hwc_data.aquireFenceCnt; i++)
	    {
            disp = composer_priv.firstdisp;
            if(i >= data->hwc_data.firstDispFenceCnt && data->hwc_data.firstDispFenceCnt != 0)
            {
                disp = (composer_priv.firstdisp ? 0 : 1);
            }
            if(composer_priv.display_active[disp] != 0)
            {
                AcquireFence =(struct sync_fence *) data->fence_array[i];
                if(AcquireFence != NULL)
                {
                    err = sync_fence_wait(AcquireFence,3000);
                    sync_fence_put(AcquireFence);
                    if (err < 0)
	                {
                        printk("synce_fence_wait timeout disp[%d]fence:%p\n",disp,AcquireFence);
                        printk("androidfrmnum:%d  ker:%d  timeline:%d\n",data->hwc_data.androidfrmnum,data->psSyncInfo->timelinenum,composer_priv.relseastimeline->value);
                        composer_priv.Cur_Write_Cnt = data->psSyncInfo->timelinenum;
                        sw_sync_timeline_inc(composer_priv.relseastimeline, 1);
                        if(data->WB_data != NULL)
                        {
                            kfree(data->WB_data);
                        }
					    goto free;
	                }
                }
            }
	    }

        if(data->psSyncInfo->timelinenum - composer_priv.Cur_Disp_Cnt[0] > 1)
        {
            wait_event_interruptible_timeout(composer_priv.waite_for_vsync,data->psSyncInfo->timelinenum - composer_priv.Cur_Disp_Cnt[0] <= 1,30);
        }
        if(data->psSyncInfo->timelinenum - composer_priv.Cur_Disp_Cnt[0] > 1)
        {
            printk("hwc skip a frame :%d %d\n",data->psSyncInfo->timelinenum,composer_priv.Cur_Disp_Cnt[0]);
        }
        //dev_composer_debug(&data->hwc_data, data->hwc_data.androidfrmnum);
        //psWBdata = hwc_setup_wbdata(data->WB_data,data->psSyncInfo);
        if(composer_priv.pause == 0)
        {
            dispc_gralloc_queue(&data->hwc_data, data->psSyncInfo, psWBdata);
        }
free:
        if(data->fence_array !=NULL)
        {
            kfree(data->fence_array);
        }
        if(data->WB_data == NULL)
        {
            kfree(data->psSyncInfo);
        }
        kmem_cache_free(composer_priv.disp_slab, data);
    }
	mutex_unlock(&(gcommit_mutek));
}

static bool hwc_get_fence(struct setup_dispc_data *disp_data,struct sync_fence ***fencefd)
{
    int cout = 0, coutoffence = 0,forwhile = 0,cnt = 0;
    bool samefence = 0;
    int *psfencefd = NULL;
    struct  sync_fence **fence_array = NULL; 
    struct sync_fence *fence = NULL;
    if(disp_data->aquireFenceCnt > 0)
    {
        psfencefd = (int *)kzalloc(( disp_data->aquireFenceCnt * sizeof(int)),GFP_KERNEL);
        fence_array = (struct sync_fence **)kzalloc(( disp_data->aquireFenceCnt * sizeof(struct sync_fence *)),GFP_KERNEL);
        if(psfencefd == NULL || fence_array == NULL)
        {
            printk("hwc kzalloc err\n");
        }
        if(copy_from_user( psfencefd, (void __user *)disp_data->aquireFenceFd, disp_data->aquireFenceCnt * sizeof(int)))
        {
            printk("copy_from_user fail\n");
            goto err;
        }
        cnt = disp_data->firstDispFenceCnt;
        for(cout = 0; cout < disp_data->aquireFenceCnt; cout++)
        {
            fence = sync_fence_fdget(psfencefd[cout]);
            if(!fence)
            {
                printk("sync_fence_fdget failed,fd[%d]:%d\n",cout, psfencefd[cout]);
                if(cout < cnt)
                {
                    disp_data->firstDispFenceCnt--;
                }
                continue;
            }
            forwhile = coutoffence;
            while(forwhile)
            {
                if(fence == fence_array[forwhile])
                {
                    samefence = 1;
                    if(cout < cnt)
                    {
                        disp_data->firstDispFenceCnt--;
                    }
                    break;
                }
                forwhile--;
            }
            if(samefence)
            {
                samefence = 0;
                continue;
            }
            fence_array[coutoffence] = fence;
            coutoffence++;
        }
    }
    disp_data->aquireFenceCnt = coutoffence;
    *fencefd = fence_array;
    kfree(psfencefd);
    return 0;
err:
    if(fence_array != NULL)
    {
        kfree(fence_array);
    }
    if(psfencefd != NULL)
    {
        kfree(psfencefd);
    }
    return -1;
}

static int hwc_assign_wb(struct setup_dispc_data *disp_data, struct WB_data_list **WB_data, struct hwc_sync_info *psSyncInfo)
{
    int fd = -1;
    struct sync_fence *fence = NULL;
	struct sync_pt *pt;
    struct WB_data_list *psWB_data = NULL;
    psWB_data = kzalloc(sizeof(struct WB_data_list),GFP_KERNEL);
    if(psWB_data == NULL)
    {
        goto err_assign_wb;
    }
    if(copy_from_user(&psWB_data->WriteBackdata, (void __user *)disp_data->WriteBackdata, sizeof(struct WriteBack)))
    {
        printk("copy_from_user WB_data fail\n");
        goto err_assign_wb;
    }

    if(disp_data->needWB[1] && !disp_data->needWB[0])
    {
        if(psWB_data->WriteBackdata.syncfd >= 0)
        {
            psWB_data->outaquirefence = sync_fence_fdget(psWB_data->WriteBackdata.syncfd);
        }
        fd = get_unused_fd();
        composer_priv.WB_count++;
        pt = sw_sync_pt_create(composer_priv.writebacktimeline, composer_priv.WB_count);
        fence = sync_fence_create("sunxi_WB", pt);
        sync_fence_install(fence, fd);
        psSyncInfo->androidOrWB = composer_priv.WB_count;
    }
    if(disp_data->needWB[0])
    {
        if(composer_priv.layer_info == NULL)
        {
            composer_priv.layer_info = kzalloc(sizeof(struct disp_layer_config)*8,GFP_KERNEL);
        }
    }
    psWB_data->psSyncInfo = psSyncInfo;
    psWB_data->isforHDMI = disp_data->needWB[0];
    *WB_data = psWB_data;
    return fd;

err_assign_wb:
    printk("hwc fix an err...\n");
    if(psWB_data != NULL)
    {
        kfree(psWB_data);
    }
    psWB_data = NULL;
    *WB_data = NULL; 
    return -1;
}

static int hwc_commit(struct setup_dispc_data *disp_data)
{
	struct dispc_data_list *disp_data_list = NULL;
	struct sync_fence *fence = NULL;
	struct sync_pt *pt = NULL;
	int fd[2] = {-1,-1};
    struct sync_fence **sync_fence_array = NULL;
    struct WB_data_list   *WB_data = NULL;
    struct hwc_sync_info    *psSyncInfo = NULL;
    if(hwc_get_fence(disp_data,&sync_fence_array))
    {
        printk("hwc get fence err\n");
        goto err;
    }
	if(disp_data->layer_num[0]+disp_data->layer_num[1] > 0 || disp_data->forceflip)
	{
        hwc_check_alive(disp_data);
        fd[0] = get_unused_fd();
        if (fd < 0)
        {
            printk("get unused fd faild\n");
            goto err;
        }
        composer_priv.timeline_max++;
        pt = sw_sync_pt_create(composer_priv.relseastimeline, composer_priv.timeline_max);
        fence = sync_fence_create("sunxi_display", pt);
        sync_fence_install(fence, fd[0]);

        disp_data_list = (struct dispc_data_list *)kmem_cache_zalloc(composer_priv.disp_slab,GFP_KERNEL);
        psSyncInfo = kzalloc(sizeof(struct hwc_sync_info), GFP_KERNEL);
        if(disp_data_list == NULL || psSyncInfo == NULL)
        {
            printk("hwc kzalloc disp_data_list[%p] psSyncInfo[%p]\n",disp_data_list,psSyncInfo);
            goto err;
        }
        psSyncInfo->timelinenum = composer_priv.timeline_max;
        psSyncInfo->androidOrWB = disp_data->androidfrmnum;
        psSyncInfo->vsyncVeryfy = composer_priv.timeline_max;
        if((disp_data->needWB[0] || disp_data->needWB[1])&& !composer_priv.b_no_output)
        {
            fd[1] = hwc_assign_wb(disp_data,&WB_data,psSyncInfo);
        }
        memcpy(&disp_data_list->hwc_data, disp_data, sizeof(struct setup_dispc_data));
        disp_data_list->psSyncInfo = psSyncInfo;
        disp_data_list->WB_data = WB_data;
        disp_data_list->fence_array = sync_fence_array;
        mutex_lock(&(composer_priv.update_regs_list_lock));
        list_add_tail(&disp_data_list->list, &composer_priv.update_regs_list);
        mutex_unlock(&(composer_priv.update_regs_list_lock));
        if(!composer_priv.pause)
        {
            queue_work(composer_priv.Display_commit_work, &composer_priv.commit_work);
        }
	}else{
	    printk("No layer from android set\n");
	    goto err;
    }
    if(composer_priv.b_no_output)
    {
        flush_workqueue(composer_priv.Display_commit_work);
#if defined(HW_FOR_HWC)
        if(!list_empty_careful(&composer_priv.WB_list))
        {
            flush_workqueue(composer_priv.Display_WB_work);
            wb_queue(0,FREE_ALL,0);
        }
#endif
    }
    if(copy_to_user((void __user *)disp_data->returnfenceFd, fd, sizeof(int)*2))
    {
	    printk("copy_to_user fail\n");
        goto err;
	}
	return 0;
err:
    if(WB_data != NULL)
    {
       kfree(WB_data);
    }
    if(psSyncInfo != NULL)
    {
        kfree(psSyncInfo);
    }
    if(disp_data_list != NULL)
    {
        kmem_cache_free(composer_priv.disp_slab, disp_data_list);
    }
    if(sync_fence_array != NULL)
    {
        kfree(sync_fence_array);
    }
    return -EFAULT;
}

static int hwc_commit_ioctl(unsigned int cmd, unsigned long arg)
{
    int ret = -EFAULT;
    //unsigned int    sync;
    char alligned[2];
	if(DISP_HWC_COMMIT == cmd)
    {
        struct hwc_ioctl_arg   hwc_cmd;
        unsigned long *ubuffer;
        ubuffer = (unsigned long *)arg;
        if(copy_from_user(&hwc_cmd, (void __user *)ubuffer[1], sizeof(struct hwc_ioctl_arg)))
        {
            printk("hwc_cmd copy_from_user fail\n");
            return  -EFAULT;
        }
        switch(hwc_cmd.cmd)
        {
            case HWC_IOCTL_COMMIT:
                memset(composer_priv.tmptransfer, 0, sizeof(struct setup_dispc_data));
                if(copy_from_user(composer_priv.tmptransfer, (void __user *)hwc_cmd.arg, sizeof(struct setup_dispc_data)))
                {
                    printk("copy_from_user fail\n");
                    return  -EFAULT;
		        }
                ret = hwc_commit(composer_priv.tmptransfer);
            break;
#if defined(HW_FOR_HWC)
            case HWC_IOCTL_WBSYNC:
                 sync = hwc_get_sync();
                 if(copy_to_user((void __user *)hwc_cmd.arg, &sync , sizeof(unsigned int)))
                 {
                    ret = -EFAULT;
                 }else{
                    ret = 0;
                 }
            break;
#endif
            case HWC_IOCTL_ALLIGN:

                 if(copy_from_user(alligned, (void __user *)hwc_cmd.arg , 2*sizeof(unsigned char)))
                 {
                    ret = -EFAULT;
                 }else{
                   composer_priv.hw_allined = alligned[0];
                   composer_priv.yuv_alligned = alligned[1];
                 }    
            break;
            default:
                printk("hwc get an err cmd\n");
        }
	}
	return ret;
}
#if defined(HW_FOR_HWC)
static void hwc_query_wb(unsigned int framenm)
{
    static int lastqueryed;
    int ret = 0;
    struct WB_data_list *data, *next;
    disp_drv_info *psg_disp_drv = composer_priv.psg_disp_drv;
    struct disp_manager *psmgr = NULL;
    struct disp_capture *psWriteBack = NULL;
    psmgr = psg_disp_drv->mgr[0];
    psWriteBack = psmgr->cptr;
    if(framenm == lastqueryed)
    {
        return ;
    }else{
        lastqueryed = framenm;
    }
    ret = psWriteBack->query(psWriteBack);
    list_for_each_entry_safe(data, next, &composer_priv.WB_list, list)
    {
        if(data->psSyncInfo->timelinenum == framenm && data->psSyncInfo->vsyncVeryfy == framenm)
        {
            data->psSyncInfo->vsyncVeryfy = ret;
        }
    }  
}


static bool hwc_setup_rotate(struct disp_s_frame *psInCapture, struct disp_s_frame *psOutCaptuer,int rotate)
{
#if defined(HW_ROTATE_MOD)
    tr_info tr;
    memset(&tr,0,sizeof(tr_info));
    switch(rotate)
    {
        case HAL_TRANSFORM_ROT_90:
            tr.mode = TR_ROT_90;
        break;
        case HAL_TRANSFORM_ROT_180:
            tr.mode = TR_ROT_180;
        break;
        case HAL_TRANSFORM_ROT_270:
            tr.mode = TR_ROT_270;
        break;
        default:
            tr.mode = TR_ROT_0;
    }
    tr.src_frame.fmt = psInCapture->format;
    tr.src_frame.laddr[0] = psInCapture->addr[0];
    tr.src_frame.laddr[1] = psInCapture->addr[1];
    tr.src_frame.laddr[2] = psInCapture->addr[2];
    tr.src_frame.pitch[0] = psInCapture->size[0].width;
    tr.src_frame.pitch[1] = psInCapture->size[1].width;
    tr.src_frame.pitch[2] = psInCapture->size[2].width;
    tr.src_frame.height[0] = psInCapture->size[0].height;
    tr.src_frame.height[1] = psInCapture->size[1].height;
    tr.src_frame.height[2] = psInCapture->size[2].height;
    tr.src_rect.x = 0;
    tr.src_rect.y = 0;
    tr.src_rect.w = psInCapture->size[0].width;
    tr.src_rect.h = psInCapture->size[0].height;
    tr.dst_frame.fmt = psOutCaptuer->format;
    tr.dst_frame.laddr[0] = psOutCaptuer->addr[0];
    tr.dst_frame.laddr[1] = psOutCaptuer->addr[1];
    tr.dst_frame.laddr[2] = psOutCaptuer->addr[2];
    tr.dst_frame.pitch[0] = psOutCaptuer->size[0].width;
    tr.dst_frame.pitch[1] = psOutCaptuer->size[1].width;
    tr.dst_frame.pitch[2] = psOutCaptuer->size[2].width;
    tr.dst_frame.height[0] = psOutCaptuer->size[0].height;
    tr.dst_frame.height[1] = psOutCaptuer->size[1].height;
    tr.dst_frame.height[2] = psOutCaptuer->size[2].height;
    tr.dst_rect.x = 0;
    tr.dst_rect.y = 0;
    tr.dst_rect.w =psOutCaptuer->size[0].width;
    tr.dst_rect.h = psOutCaptuer->size[0].height;
    if(composer_priv.tr_fd == 0)
    {
        printk("tr_fd is [0] err...\n");
        goto err_rotate;
    }
    tr.fd = composer_priv.tr_fd;
    sunxi_tr_commit(tr.fd,&tr);
    msleep(3);
    if(sunxi_tr_query(tr.fd) == 1)
    msleep(1);
#endif
    return 0;
err_rotate:
    return 1;
}

static inline bool hwc_copy_memory(struct disp_s_frame * psout_put, struct disp_s_frame * psin_put)
{
    void *dstvaddr = NULL, *srcvaddr = NULL;
    struct format_info format_info;
    int srcsize = 0, dstsize = 0;
    bool ret = 0;

    if(hwc_format_info(&format_info,psin_put->format))
    {
        goto err_ctl;
    }
    srcsize = psin_put->size[0].width * psin_put->size[0].height * format_info.bpp / 8;
    if(hwc_format_info(&format_info,psout_put->format))
    {
        goto err_ctl;
    }
    dstsize = psout_put->size[0].width * psout_put->size[0].height * format_info.bpp / 8;
    srcvaddr = sunxi_map_kernel(psin_put->addr[0],srcsize);
    dstvaddr = sunxi_map_kernel(psout_put->addr[0],dstsize);
    if(srcvaddr == NULL || dstvaddr == NULL)
    {
        ret = 1;
        goto err_ctl;
    }
    memcpy(srcvaddr,dstvaddr,dstsize >= dstsize ? dstsize : dstsize);
err_ctl:
    if(srcvaddr != NULL)
    {
        sunxi_unmap_kernel(srcvaddr);
    }
    if(dstvaddr != NULL)
    {
        sunxi_unmap_kernel(dstvaddr);
    }
    return ret;
}

static inline void hwc_wb_errctl(struct WB_data_list *wb_list,bool err)
{
    if(!err)
    {
        bool ret = 0;
        struct WB_data_list *data, *next;
        if(list_empty_careful(&composer_priv.WB_err_list))
        {
            return;
        }
        list_for_each_entry_safe(data, next, &composer_priv.WB_err_list, list)
        {
            ret = hwc_copy_memory(&data->WriteBackdata.capturedata.out_frame,&wb_list->WriteBackdata.capturedata.out_frame);
            if(!ret)
            {
                list_del(&data->list);
            }
        }
    }else{
            list_add_tail(&wb_list->list, &composer_priv.WB_err_list);
    }
}

static void hwc_write_back(struct work_struct *work)
{
    struct WB_data_list *data, *next;
    struct disp_layer_config   *layer_info = NULL;
    struct disp_manager *psmgr = NULL;
    struct disp_s_frame *psout_frame = NULL;
    struct hwc_sync_info *psSyncInfo = NULL;
    int err = 0, whilecnt = 5;
    unsigned int start_cnt = 0;
    struct wb_layer   *wb_rotate = NULL;
    bool free_wq = 0;
    struct disp_rectsz screen;
    struct disp_rect crop;
    disp_drv_info *psg_disp_drv = composer_priv.psg_disp_drv;
    struct disp_rect *pswindow = NULL;
    struct format_info format_info;

    mutex_lock(&(gwb_mutek));
    list_for_each_entry_safe(data, next, &composer_priv.WB_list, list)
    {
        err = 0;
        free_wq = 0;
        psmgr = NULL;
        psout_frame = NULL;
        pswindow = NULL;
        wb_rotate = NULL;
        if(composer_priv.last_wb_cnt >= data->psSyncInfo->timelinenum || composer_priv.b_no_output)
        {
            psSyncInfo = data->psSyncInfo;
            if(psSyncInfo->vsyncVeryfy != 0 || composer_priv.b_no_output)
            {
                err = 1;
                free_wq = 1;
                goto ret_ctl;
            }
            psout_frame = &data->WriteBackdata.capturedata.out_frame;
            if(data->pscapture != NULL)
            {
                if(data->outaquirefence != NULL && !data->outaquirefence->status)
                {
                    err = sync_fence_wait(data->outaquirefence,3000);
                    sync_fence_put(data->outaquirefence);
                    if (err < 0)
	                {
                        printk("hwc wait for wb fence err.\n");
					    goto ret_ctl;
	                }
                }
                if(data->isforHDMI)
                {
                    if((composer_priv.last_wb_cnt == data->psSyncInfo->timelinenum) 
                        && data->pscapture->out_frame.addr[0] != 0)
                    {
                        layer_info = &composer_priv.layer_info[0];
                        memset(layer_info,0,sizeof(disp_layer_config)*8);
                        if(data->WriteBackdata.rotate)
                        {
                            pswindow = &data->pscapture->window;
                            if(data->WriteBackdata.rotate & HAL_TRANSFORM_ROT_90)
                            {
                                screen.width = pswindow->height;
                                screen.height = pswindow->width;
                                crop.x = pswindow->y;
                                crop.y = pswindow->x;
                                crop.width = pswindow->height;
                                crop.height = pswindow->width;
                            }else{
                                screen.width = pswindow->width;
                                screen.height = pswindow->height;
                                crop.x = pswindow->x;
                                crop.y = pswindow->y;
                                crop.width = pswindow->width;
                                crop.height = pswindow->height;
                            }
                            wb_rotate = wb_dequeue(screen, crop, data->pscapture->out_frame.format, data->psSyncInfo->timelinenum, 1);
                            if(wb_rotate == NULL)
                            {
                                err = 1;
                                free_wq = 1;
                                printk("hwc wb_rotate dequeue err\n");
                                goto ret_ctl;
                            }
                        }
                        if(wb_rotate != NULL)
                        {
                            hwc_setup_layer(layer_info, psout_frame, &wb_rotate->screen,(unsigned int)wb_rotate->p_addr);
                            hwc_format_info(&format_info,data->pscapture->out_frame.format);
                            hwc_setup_capture(psout_frame, &format_info, screen, crop, wb_rotate);
                        }else{
                            hwc_setup_layer(layer_info, psout_frame, &data->pscapture->out_frame.size[0],data->pscapture->out_frame.addr[0]);
                        }
                    }else{
                        err = 1;
                        free_wq = 1;
                        goto ret_ctl;
                    }
                }
                if(data->WriteBackdata.rotate)
                {
                    err = hwc_setup_rotate(&data->pscapture->out_frame,psout_frame,data->WriteBackdata.rotate);
                    if(err)
                    {
                        free_wq = 1;
                        printk("hwc rotate has a err\n");
                        goto ret_ctl;
                    }
                }else if(!data->isforHDMI){
                    err = hwc_copy_memory(psout_frame,&data->pscapture->out_frame);
                    if(err)
                    {
                        free_wq = 1;
                        printk("hwc has a memcopy err\n");
                        goto ret_ctl;
                    }
                }
                if(data->isforHDMI)
                {
                    psmgr = psg_disp_drv->mgr[1];
                    if(psmgr != NULL && composer_priv.controlbywb)
                    {
                        bsp_disp_shadow_protect(1,true);
                        psmgr->set_layer_config(psmgr, layer_info, 8);
                        bsp_disp_shadow_protect(1,false);
                        composer_priv.Cur_Disp2_WB = data->psSyncInfo->timelinenum;
                        start_cnt= composer_priv.Cur_Disp_wb;
                        whilecnt = 5;
                        while(start_cnt + 1 != data->psSyncInfo->timelinenum)
                        {
                            wb_queue(++start_cnt,FREE_PREC,0);
                            wb_queue(start_cnt,FREE_PREC,1);
                            if(whilecnt--)
                            {
                                break;
                            }
                        }
                    }
                }
            }

ret_ctl:
            spin_lock(&composer_priv.WB_lock);
            list_del(&data->list);
            spin_unlock(&composer_priv.WB_lock);
            if(!data->isforHDMI)
            {
                hwc_wb_errctl(data,err);
            }else{
                err = 1;
            }
            if(free_wq)
            {
                wb_queue(data->psSyncInfo->timelinenum,FREE_PREC,0);
            }
            if(data->pscapture != NULL)
            {
                kfree(data->pscapture);
            }
            if(data->psSyncInfo != NULL)
            {
                kfree(data->psSyncInfo);
            }
            if(!err)
            {
                while(!data->isforHDMI && composer_priv.writebacktimeline->value != data->psSyncInfo->androidOrWB)
                {
                    sw_sync_timeline_inc(composer_priv.writebacktimeline, 1);
                }
                kfree(data);
            }
        }else{
            break;
        }
    }
    mutex_unlock(&(gwb_mutek));
    return ;
}
#endif
static void post2_cb(struct work_struct *work)
{
	mutex_lock(&composer_priv.runtime_lock);
    imp_finish_cb(composer_priv.b_no_output);
	mutex_unlock(&composer_priv.runtime_lock);
}

extern s32  bsp_disp_shadow_protect(unsigned int disp, bool protect);

static void imp_finish_cb(bool force_all)
{
    u32 little = composer_priv.Cur_Write_Cnt;
	u32 flag = 0;
    bool rotate = 0;
    int secenddsp = 1;
#if defined(HW_FOR_HWC)
    disp_drv_info *psg_disp_drv = composer_priv.psg_disp_drv;
    struct disp_manager *psmgr = NULL;
    struct disp_capture *psWriteBack = NULL;
#endif
    if(composer_priv.firstdisp == 1)
    {
        secenddsp = 0;
    }

    if(composer_priv.pause)
    {
        return;
    }
    wake_up(&composer_priv.waite_for_vsync);
    if(composer_priv.display_active[composer_priv.firstdisp] == HWC_DISPLAY_ACTIVE)
    {
        little = composer_priv.Cur_Disp_Cnt[composer_priv.firstdisp];
    }
    if( (composer_priv.display_active[composer_priv.firstdisp] == HWC_DISPLAY_ACTIVE) && composer_priv.countrotate[composer_priv.firstdisp]
        &&(composer_priv.display_active[secenddsp] == HWC_DISPLAY_ACTIVE)&& composer_priv.countrotate[secenddsp])
    {
        composer_priv.countrotate[composer_priv.firstdisp] = 0;
        composer_priv.countrotate[secenddsp] = 0;
    }
    if(composer_priv.display_active[secenddsp] == HWC_DISPLAY_ACTIVE)
    {
        if( composer_priv.display_active[composer_priv.firstdisp] == HWC_DISPLAY_ACTIVE)
        {
            if(composer_priv.countrotate[composer_priv.firstdisp] != composer_priv.countrotate[1])
            {
                if(composer_priv.countrotate[composer_priv.firstdisp] && (composer_priv.display_active[composer_priv.firstdisp]== HWC_DISPLAY_ACTIVE))
                {
                    little = composer_priv.Cur_Disp_Cnt[secenddsp];
                }else{
                    little = composer_priv.Cur_Disp_Cnt[composer_priv.firstdisp];
                }
            }else{
                if(composer_priv.Cur_Disp_Cnt[secenddsp] > composer_priv.Cur_Disp_Cnt[composer_priv.firstdisp])
                {
                    little = composer_priv.Cur_Disp_Cnt[composer_priv.firstdisp];
                }else{
                    little = composer_priv.Cur_Disp_Cnt[secenddsp];
                }
            }
      }else{
            little = composer_priv.Cur_Disp_Cnt[secenddsp];
      }
    }
    while(composer_priv.relseastimeline->value != composer_priv.Cur_Write_Cnt)
    {
        if(composer_priv.relseastimeline->value > composer_priv.Cur_Write_Cnt)
        {
            rotate = 1;
        }
        if(rotate && composer_priv.relseastimeline->value > little - 1)
        {
            rotate = 1;
        }else{
            rotate = 0;
        }
        if( !force_all
            &&(rotate == 0 ? composer_priv.relseastimeline->value >= little - 1
                           : composer_priv.relseastimeline->value == little - 1 ))
        {
            break;
        }
        sw_sync_timeline_inc(composer_priv.relseastimeline, 1);
        composer_frame_checkin(1);
        flag = 1;
    }
    if(flag)
    {
		composer_frame_checkin(2);
    }
#if defined(HW_FOR_HWC)
    if(force_all && !list_empty_careful(&composer_priv.WB_list))
    {
        flush_workqueue(composer_priv.Display_WB_work);
        wb_queue(0,FREE_ALL,0);
        wb_free();
        psmgr = psg_disp_drv->mgr[0];
        psWriteBack = psmgr->cptr;
        if(composer_priv.WB_status[0] == 1)
        {
            psWriteBack->stop(psWriteBack);
            composer_priv.WB_status[0] = 0;
        }
    }
#endif
}

static void disp_composer_proc(unsigned int sel)
{

    if(sel < 2)
    {
        if(composer_priv.Cur_Write_Cnt < composer_priv.Cur_Disp_Cnt[sel])
        {
            composer_priv.countrotate[sel] = 1;
        }
#if defined(HW_FOR_HWC)
        if(!list_empty_careful(&composer_priv.WB_list))
        {
            if(sel == 0)
            {
                hwc_query_wb(composer_priv.Cur_Disp_Cnt[0]);
                composer_priv.last_wb_cnt = composer_priv.Cur_Disp_Cnt[0];
                queue_work(composer_priv.Display_WB_work, &composer_priv.WB_work);
            }
            if(sel == 1)
            {
                composer_priv.Cur_Disp_wb = composer_priv.Cur_Disp2_WB;
                wb_queue(composer_priv.Cur_Disp_wb,FREE_LITTLE,0);
            }
        }
#endif
        composer_priv.Cur_Disp_Cnt[sel] = composer_priv.Cur_Write_Cnt;
    }

	schedule_work(&composer_priv.post2_cb_work);
	return ;
}

int dispc_gralloc_queue(struct setup_dispc_data *psDispcData, struct hwc_sync_info *hwc_sync, struct disp_capture_info *psWBdata)
{
    int disp;
    disp_drv_info *psg_disp_drv = composer_priv.psg_disp_drv;
    struct disp_manager *psmgr = NULL;
    struct disp_enhance *psenhance = NULL;
    struct disp_capture *psWriteBack = NULL;
    struct disp_layer_config *psconfig;
    disp = 0;
    while( disp < DISP_NUMS_SCREEN )
    {
        if(!composer_priv.display_active[disp] || (disp ==1 && composer_priv.controlbywb))
        {
            disp++;
            continue;
        }
        psmgr = psg_disp_drv->mgr[disp];
        psenhance = psmgr->enhance;
        psWriteBack = psmgr->cptr;
        if( psmgr != NULL )
        {
            bsp_disp_shadow_protect(disp,true);
            if(disp == 0)
            {
                if(hwc_sync->vsyncVeryfy != hwc_sync->timelinenum )
                {
                    if(hwc_sync->vsyncVeryfy == composer_priv.Cur_Disp_Cnt[0])
                    {
                        hwc_sync->vsyncVeryfy = hwc_sync->timelinenum;
                    }
                }
            }
            if(psDispcData->ehancemode[disp] )
            {
                if(psDispcData->ehancemode[disp] != composer_priv.ehancemode[disp])
                {
                    switch (psDispcData->ehancemode[disp])
                    {
                        case 1:
                            psenhance->demo_disable(psenhance);
                            psenhance->enable(psenhance);
                        break;
                        case 2:
                            psenhance->enable(psenhance);
                            psenhance->demo_enable(psenhance);
                        break;
                        default:
                            psenhance->disable(psenhance);
                            printk("translat a err info\n");
                    }
                }
                composer_priv.ehancemode[disp] = psDispcData->ehancemode[disp];
            }else{
                if(composer_priv.ehancemode[disp])
                {
                    psenhance->disable(psenhance);
                    composer_priv.ehancemode[disp] = 0;
                }
            }
            if(disp == 0)
            {
                if(psWBdata != NULL && !composer_priv.b_no_output)
                {
                    if(composer_priv.WB_status[0] != 1)
                    {
                        psWriteBack->start(psWriteBack);
                        composer_priv.WB_status[0] = 1;
                    }
                    psWriteBack->commmit(psWriteBack,psWBdata);
                }else{
                    if(list_empty(&composer_priv.WB_list))
                    {
                        if(composer_priv.WB_status[0] == 1)
                        {
                            psWriteBack->stop(psWriteBack);
                            composer_priv.WB_status[0] = 0;
                        }
                    }
                }
            }
            psconfig = &psDispcData->layer_info[disp][0];
            psmgr->set_layer_config(psmgr, psconfig, disp? 8 : 16);
            bsp_disp_shadow_protect(disp,false);
            if(composer_priv.display_active[disp] == HWC_DISPLAY_PREACTIVE)
            {
                composer_priv.display_active[disp] = HWC_DISPLAY_ACTIVE;
                composer_priv.Cur_Disp_Cnt[disp] = hwc_sync->timelinenum;
            }
        }
        disp++;
    }
    composer_priv.Cur_Write_Cnt = hwc_sync->timelinenum;
    composer_frame_checkin(0);
    if(composer_priv.b_no_output)
    {
        mutex_lock(&composer_priv.runtime_lock);
	    imp_finish_cb(1);
	    mutex_unlock(&composer_priv.runtime_lock);
    }
  return 0;
}

static int hwc_suspend(void)
{
	composer_priv.b_no_output = 1;
	mutex_lock(&composer_priv.runtime_lock);
	imp_finish_cb(1);
	mutex_unlock(&composer_priv.runtime_lock);
	printk("%s\n", __func__);
	return 0;
}

static int hwc_resume(void)
{
	composer_priv.b_no_output = 0;
	printk("%s\n", __func__);
	return 0;
}
#if defined(HW_FOR_HWC)
static struct dentry *composer_pdbg_root;

static int dumplayer_open(struct inode * inode, struct file * file)
{
	return 0;
}

static int dumplayer_release(struct inode * inode, struct file * file)
{
	return 0;
}

static ssize_t dumplayer_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    struct dumplayer *dmplyr = NULL, *next = NULL;
    composer_priv.dumpstart = 1;

    while(composer_priv.dumpCnt != 0)
    {
        list_for_each_entry_safe(dmplyr, next, &composer_priv.dumplyr_list, list)
        {
            if(dmplyr->update)
            {
                debug_write_file(dmplyr);
            }
        }
    }
    printk("dumplist counter %d\n",composer_priv.listcnt);
    list_for_each_entry_safe(dmplyr, next, &composer_priv.dumplyr_list, list)
    {
        list_del(&dmplyr->list);
        if(dmplyr->update)
        {
            debug_write_file(dmplyr);
        }
        if(dmplyr->isphaddr)
        {
            sunxi_unmap_kernel(dmplyr->vm_addr);
            sunxi_mem_free((unsigned int)dmplyr->p_addr,dmplyr->size);
        }else{
            kfree(dmplyr->vm_addr);
        }
        kfree(dmplyr);
    }
    composer_priv.dumpstart = 0;
	return 0;
}

static ssize_t dumplayer_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    struct dumplayer *dmplyr = NULL, *next = NULL;
    char temp[count+1];
    char *s = temp;
    int cnt=0;
    if(copy_from_user( temp, (void __user *)buf, count))
    {
        printk("copy_from_user fail\n");
        return  -EFAULT;
    }
    temp[count] = '\0';
    printk("%s\n",temp);
    switch(*s++)
    {
        case 'p':
            composer_priv.pause = 1;
        break;
        case 'q':
            composer_priv.pause = 0;
            composer_priv.cancel = 0;
            composer_priv.dumpstart = 0;
            composer_priv.dumpCnt = 0;
            list_for_each_entry_safe(dmplyr, next, &composer_priv.dumplyr_list, list)
            {
                list_del(&dmplyr->list);
                if(dmplyr->isphaddr)
                {
                    sunxi_unmap_kernel(dmplyr->vm_addr);
                    sunxi_mem_free((unsigned int)dmplyr->p_addr,dmplyr->size);
                }else{
                    kfree(dmplyr->vm_addr);
                }
                kfree(dmplyr);
            }
        break;
        case 'c':
            composer_priv.cancel = 1;
        case 'd':
            composer_priv.pause = 0;
            composer_priv.display = (*s >= 48 && *s <=49)? *s - 48 : 255;
            s++;
            if(*s == 'c')
            {
                s++;
                composer_priv.channelNum = (*s >= 48 && *s <=51)? *s - 48 : 255;
            }else{
                composer_priv.dumpCnt = 0;
                break;
            }
            s++;
            if(*s == 'l')
            {
                s++;
                composer_priv.layerNum = (*s >= 48 && *s <=51)? *s - 48 : 255;
            }else{
                composer_priv.dumpCnt = 0;
                break;
            }
            s++;
            if(*s == 'n')
            {
                while(*++s != '\0')
                {
                    if( 57< *s || *s <48)
                        break;
                    cnt += (*s-48);
                    cnt *=10;
                }
                composer_priv.dumpCnt = cnt/10;
            }else{
                composer_priv.dumpCnt = 0;
            }
            composer_priv.listcnt = 0;
        break;
        default:
            printk(" dev_composer debug give me a wrong arg...\n");
    }
    printk("dumps --Cancel[%d]--display[%d]--channel[%d]--layer[%d]--Cnt:[%d]--pause[%d] \n",composer_priv.cancel,composer_priv.display,composer_priv.channelNum,composer_priv.layerNum,composer_priv.dumpCnt,composer_priv.pause);
    return count;
}

static const struct file_operations dumplayer_ops = {
	.write        = dumplayer_write,
	.read        = dumplayer_read,
	.open        = dumplayer_open,
	.release    = dumplayer_release,
};
int composer_dbg(void)
{
	composer_pdbg_root = debugfs_create_dir("composerdbg", NULL);
	if(!debugfs_create_file("dumplayer", 0644, composer_pdbg_root, NULL,&dumplayer_ops))
		goto Fail;
	return 0;

Fail:
	debugfs_remove_recursive(composer_pdbg_root);
	composer_pdbg_root = NULL;
	return -ENOENT;
}
#endif
int composer_init(disp_drv_info *psg_disp_drv)
{

	memset(&composer_priv, 0x0, sizeof(struct composer_private_data));
	INIT_WORK(&composer_priv.post2_cb_work, post2_cb);
	mutex_init(&composer_priv.runtime_lock);
    mutex_init(&composer_priv.queue_lock);
    composer_priv.Display_commit_work = create_freezable_workqueue("SunxiDisCommit");
    composer_priv.Display_WB_work = create_freezable_workqueue("Sunxi_WB");
    INIT_WORK(&composer_priv.commit_work, hwc_commit_work);
    //INIT_WORK(&composer_priv.WB_work, hwc_write_back);

	INIT_LIST_HEAD(&composer_priv.update_regs_list);
    INIT_LIST_HEAD(&composer_priv.dumplyr_list);
    INIT_LIST_HEAD(&composer_priv.WB_list);
    INIT_LIST_HEAD(&composer_priv.WB_err_list);
    INIT_LIST_HEAD(&composer_priv.WB_miracast_list);

	composer_priv.relseastimeline = sw_sync_timeline_create("sunxi-display");
    composer_priv.writebacktimeline = sw_sync_timeline_create("Sunxi-WB");
    composer_priv.WB_count = 0;
	composer_priv.timeline_max = 0;
	composer_priv.b_no_output = 0;
    composer_priv.Cur_Write_Cnt = 0;
    composer_priv.display_active[0] = 0;
    composer_priv.display_active[1] = 0;
    composer_priv.Cur_Disp_Cnt[0] = 0;
    composer_priv.Cur_Disp_Cnt[1] = 0;
    composer_priv.countrotate[0] = 0;
    composer_priv.countrotate[1] = 0;
    composer_priv.pause = 0;
    composer_priv.listcnt = 0;
    composer_priv.last_wb_cnt = 0;
    composer_priv.tr_fd = 0;
    composer_priv.initrial = 0;
    composer_priv.controlbywb = 0;
    composer_priv.max_queue = 4;
    composer_priv.layer_info = NULL;
    composer_priv.wb_queue = NULL;
    composer_priv.queue_num = 0;
    composer_priv.yuv_alligned = 16;
    composer_priv.hw_allined = 32;

    composer_priv.disp_slab = kmem_cache_create("display_data",sizeof(struct dispc_data_list),0,0,NULL);
    //composer_priv.sync_info_slab = kmem_cache_create("sync info",sizeof(hwc_sync_info),GFP_KERNEL,NULL);
    if(composer_priv.disp_slab == NULL  )
    {
        printk("hwc creat slab err\n");
    }
    init_waitqueue_head(&composer_priv.waite_for_vsync);

	mutex_init(&composer_priv.update_regs_list_lock);
    mutex_init(&gcommit_mutek);
    mutex_init(&gwb_mutek);
	spin_lock_init(&(composer_priv.update_reg_lock));
    spin_lock_init(&(composer_priv.dumplyr_lock));
    spin_lock_init(&(composer_priv.WB_lock));
	disp_register_ioctl_func(DISP_HWC_COMMIT, hwc_commit_ioctl);
    disp_register_sync_finish_proc(disp_composer_proc);
	disp_register_standby_func(hwc_suspend, hwc_resume);
    composer_priv.tmptransfer = kzalloc(sizeof(struct setup_dispc_data),GFP_KERNEL);
    composer_priv.psg_disp_drv = psg_disp_drv;
//    composer_dbg();
    printk("hwc init  successfull\n");
    return 0;
}
#endif
