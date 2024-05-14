#include <types.h>
#include <mmap.h>
#include <fork.h>
#include <v2p.h>
#include <page.h>

/* 
 * You may define macros and other helper functions here
 * You must not declare and use any static/global variables 
 * */


/**
 * mprotect System call Implementation.
 */
long vm_area_mprotect(struct exec_context *current, u64 addr, int length, int prot)
{
    int val=0;
    if(length%PAGE_SIZE) length = (length/PAGE_SIZE +1)*PAGE_SIZE;
	if((MMAP_AREA_START > addr) || (MMAP_AREA_END < (addr+length))) return -1;
	if(PROT_WRITE==prot){
        prot=PROT_WRITE|PROT_READ;
    }
	if(current->vm_area->vm_next==NULL) return -1;
    
    struct vm_area* x=current->vm_area;
    struct vm_area* y=current->vm_area->vm_next;
    while(y!=NULL){
        if(addr+length >= y->vm_end && y->vm_start >= addr){
            y->access_flags=prot;
            x=y;
            y=y->vm_next;
        }
        else if(addr+length > y->vm_start && y->vm_start >= addr){
            stats->num_vm_area = stats->num_vm_area + 1;
            if(128<stats->num_vm_area) return -1;
            struct vm_area* temp= os_alloc(sizeof(struct vm_area));
            temp->vm_start=y->vm_start;
            temp->vm_end=addr+length;
            temp->access_flags=prot;
            y->vm_start=length+addr;
            x->vm_next=temp;
            x=y;
            temp->vm_next=x;
            y=y->vm_next;
        }
        else if(addr+length < y->vm_end && y->vm_start < addr){
            stats->num_vm_area++;
            stats->num_vm_area++;
            if(stats->num_vm_area>128) return -1;
            struct vm_area * temp = os_alloc(sizeof(struct vm_area));
            struct vm_area * temp1 = os_alloc(sizeof(struct vm_area));
            temp->vm_next=y->vm_next;
            temp->vm_end= y->vm_end;
            temp->vm_start = length + addr;
            temp->access_flags=y->access_flags;
            temp1->vm_next=temp;
            temp1->vm_end=length + addr;
            temp1->vm_start=addr;
            temp1->access_flags=prot;
            y->vm_end=addr;
            y->vm_next=temp1;
            x=temp;
            y=temp->vm_next;
        }
        else if(y->vm_start < addr && addr < y->vm_end){
            stats->num_vm_area = stats->num_vm_area+1;
            if(128 < stats->num_vm_area) return -1;
            struct vm_area* temp= os_alloc(sizeof(struct vm_area));
            temp->vm_end=y->vm_end;
            temp->vm_start=addr;
            temp->access_flags=prot;
            y->vm_end=addr;
            temp->vm_next=y->vm_next;
            y->vm_next=temp;
            x=temp;
            y=temp->vm_next;
        }
        else{
            x=y;
            y=y->vm_next;
        }
    }
    y = current->vm_area;
    x = NULL;
    while (!(y == NULL))
    {
        if (!(x == NULL))
        {
            if (y->access_flags == x->access_flags && y->vm_start == x->vm_end)
            {
                x->vm_end = y->vm_end;
                x->vm_next = y->vm_next;
                os_free(y,sizeof(struct vm_area));
                stats->num_vm_area = stats->num_vm_area -1;
                y = x->vm_next;
                continue;
            }
        }
        x = y;
        y = y->vm_next;
    }
    return val;
}

/**
 * mmap system call implementation.
 */
long vm_area_map(struct exec_context *current, u64 addr, int length, int prot, int flags)
{
    long r_add = -1;
	if(flags!=MAP_FIXED && flags!=0) return -1;
    if(prot!= (PROT_READ | PROT_WRITE) && prot!=PROT_WRITE && prot!=PROT_READ) return -1;
	if(0>length) return -1;
    if(length%PAGE_SIZE) length = (length/PAGE_SIZE +1)*PAGE_SIZE;
	if(524288<length) return -1;
	if(PROT_WRITE == prot) prot=PROT_WRITE|PROT_READ;
	if(0==stats->num_vm_area){
		stats->num_vm_area = stats->num_vm_area+1;
		struct vm_area * dummy = os_alloc(sizeof(struct vm_area));
		dummy->vm_next=NULL;
		dummy->vm_start= MMAP_AREA_START;
		dummy->vm_end= PAGE_SIZE + dummy->vm_start;
		dummy->access_flags=0x0;
		current->vm_area=dummy;
	}
	if(!addr){
		if(flags==MAP_FIXED) return -1;
		struct vm_area* x=current->vm_area;
		struct vm_area* y=current->vm_area->vm_next;
		u64 curr_addr= current->vm_area->vm_end;
		if(y==NULL){
			stats->num_vm_area = stats->num_vm_area+1;
			struct vm_area * temp = os_alloc(sizeof(struct vm_area));
            temp->vm_start= curr_addr;
            temp->vm_next=NULL;
            temp->vm_end= temp->vm_start + length;
            temp->access_flags=prot;
			current->vm_area->vm_next=temp;
			return temp->vm_start;
		}
		while(y!=NULL){
			if(y->vm_start >= length+curr_addr){
				stats->num_vm_area++;
                struct vm_area * temp = os_alloc(sizeof(struct vm_area));
                temp->vm_next=NULL;
                temp->vm_start= curr_addr;
                temp->vm_end= temp->vm_start + length;
                temp->access_flags=prot;
				x->vm_next=temp;
				temp->vm_next=y;
				r_add= curr_addr;
				break;
			}
			curr_addr=y->vm_end;
			x=y;
			y=y->vm_next;
		}
		if(y==NULL){
			if(MMAP_AREA_END >= length + x->vm_end){
				stats->num_vm_area =1+ stats->num_vm_area;
				if(128 < stats->num_vm_area) return -1;
				struct vm_area * temp = os_alloc(sizeof(struct vm_area));
                temp->vm_next=NULL;
                temp->vm_start= x->vm_end;
                temp->vm_end= temp->vm_start + length;
                temp->access_flags=prot;
                x->vm_next=temp;
				r_add= x->vm_end;
			}
		}

		y = current->vm_area;
	        x = NULL;
        	while (!(y== NULL))
        	{
            		if (!(x== NULL))
            		{
                		if (y->access_flags == x->access_flags && y->vm_start == x->vm_end)
                		{
                		    x->vm_end = y->vm_end;
                		    x->vm_next = y->vm_next;
				            os_free(y,sizeof(struct vm_area));
                		    stats->num_vm_area = stats->num_vm_area -1;
                		    y = x->vm_next;
                		    continue;
                		}
            		}
            		x = y;
            		y = y->vm_next;
        	}
	}

	else{
		struct vm_area* y=current->vm_area->vm_next;
        struct vm_area* x=current->vm_area;
        int fl=0;
		u64 curr_addr= current->vm_area->vm_end;
        if(y==NULL){
            stats->num_vm_area = stats->num_vm_area+1;
            struct vm_area * temp = os_alloc(sizeof(struct vm_area));
            temp->vm_next=NULL;
            temp->vm_start= addr;
            temp->vm_end= temp->vm_start + length;
            temp->access_flags=prot;
            current->vm_area->vm_next=temp;
            return temp->vm_start;
        }
		while(y!=NULL){
			if(y->vm_start >= length+addr && addr >=curr_addr){
				fl=1;
				stats->num_vm_area = stats->num_vm_area+1;
                struct vm_area * temp = os_alloc(sizeof(struct vm_area));
                temp->vm_next=NULL;
                temp->vm_start= addr;
                temp->vm_end= temp->vm_start + length;
                temp->access_flags=prot;
                x->vm_next=temp;
                temp->vm_next=y;
                r_add= addr;
                break;
            }
            curr_addr=y->vm_end;
            x=y;
            y=y->vm_next;
        }
		if(y==NULL){
            if(x->vm_end<= addr && addr + length <=MMAP_AREA_END){
				fl=1;
                stats->num_vm_area = stats->num_vm_area+1;
                if(128 < stats->num_vm_area) return -1;
                struct vm_area * temp = os_alloc(sizeof(struct vm_area));
                temp->vm_next=NULL;
                temp->vm_start= addr;
                temp->vm_end= temp->vm_start + length;
                temp->access_flags=prot;
                x->vm_next=temp;
                r_add= addr;
            }
        }
		if(0==fl){
			if(MAP_FIXED==flags) return -1;
			return vm_area_map(current,0,length,prot,flags);
		}
		y = current->vm_area;
        x = NULL;
        while (y != NULL)
        {
            if (x != NULL){
                if (y->access_flags == x->access_flags && y->vm_start == x->vm_end){
                    x->vm_end = y->vm_end;
                    x->vm_next = y->vm_next;
                    os_free(y,sizeof(struct vm_area));
                    stats->num_vm_area = stats->num_vm_area-1;
                    y = x->vm_next;
                    continue;
                }
            }
            x = y;
            y = y->vm_next;
        }
	}

    return r_add;
}

/**
 * munmap system call implemenations
 */

long vm_area_unmap(struct exec_context *current, u64 addr, int length)
{
	int val=0;
    if(length%PAGE_SIZE) length = (length/PAGE_SIZE +1)*PAGE_SIZE;
    struct vm_area* x=current->vm_area;
	struct vm_area* y=current->vm_area->vm_next;
	while(y!=NULL){
		if(length+addr >= y->vm_end && y->vm_start>=addr){
			stats->num_vm_area = stats->num_vm_area-1;
			struct vm_area * temp=y;
			y=y->vm_next;
			x->vm_next=y;
			os_free(temp,sizeof(struct vm_area));
		}
		else if(y->vm_start>=addr && length+addr>y->vm_start){
            y->vm_start= length+addr;
			x=y;
			y=y->vm_next;
		}
		else if(addr+length < y->vm_end && y->vm_start < addr){
            stats->num_vm_area = stats->num_vm_area+1;
			if(128<stats->num_vm_area) return -1;
			struct vm_area * temp = os_alloc(sizeof(struct vm_area));
			temp->vm_next=y->vm_next;
			temp->vm_end= y->vm_end;
			temp->vm_start = length+addr;
			temp->access_flags=y->access_flags;
			y->vm_end=addr;
			y->vm_next=temp;
			x=temp;
			y=temp->vm_next;
		}
		else if(addr < y->vm_end && y->vm_start < addr){
			y->vm_end=addr;
			x=y;
			y=y->vm_next;
        }
		else{
			x=y;
			y=y->vm_next;
		}
    }
}



/**
 * Function will invoked whenever there is page fault for an address in the vm area region
 * created using mmap
 */

long vm_area_pagefault(struct exec_context *current, u64 addr, int error_code)
{
    return -1;
}

/**
 * cfork system call implemenations
 * The parent returns the pid of child process. The return path of
 * the child process is handled separately through the calls at the 
 * end of this function (e.g., setup_child_context etc.)
 */

long do_cfork(){
    u32 pid;
    struct exec_context *new_ctx = get_new_ctx();
    struct exec_context *ctx = get_current_ctx();
     /* Do not modify above lines
     * 
     * */   
     /*--------------------- Your code [start]---------------*/
     

     /*--------------------- Your code [end] ----------------*/
    
     /*
     * The remaining part must not be changed
     */
    copy_os_pts(ctx->pgd, new_ctx->pgd);
    do_file_fork(new_ctx);
    setup_child_context(new_ctx);
    return pid;
}



/* Cow fault handling, for the entire user address space
 * For address belonging to memory segments (i.e., stack, data) 
 * it is called when there is a CoW violation in these areas. 
 *
 * For vm areas, your fault handler 'vm_area_pagefault'
 * should invoke this function
 * */

long handle_cow_fault(struct exec_context *current, u64 vaddr, int access_flags)
{
  return -1;
}
